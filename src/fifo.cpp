#include "../include/fifo.hpp"

#include <stdexcept>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/java_symbol.hpp"

namespace ipc {

std::int64_t get_timestamp();

Fifo::Fifo(std::string path) : path_(std::move(path)) {
    buffer_ = new char[BUFFER_SIZE];
}

Fifo::~Fifo() {
    // Close pipe if open
    if (fd_ != -1) {
        close();
    }

    delete[] buffer_;
}

bool Fifo::open() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if pipe is already open
    if (fd_ != -1)
        return true;

    // Create pipe
    int res = mkfifo(path_.c_str(), 0666);
    if (!res) {
        return false;
    }

    // Open pipe
    fd_ = ::open(path_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ == -1) {
        unlink(path_.c_str());
        return false;
    }

    return true;
}

bool Fifo::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if pipe is already closed
    if (fd_ == -1)
        return false;

    // Close and unlink pipe
    unlink(path_.c_str());
    ::close(fd_);
    fd_ = -1;

    return true;
}

bool Fifo::await_data() {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    pollfd pfd{
            .fd = fd_,
            .events = POLLIN,
            .revents = 0
    };

    // Poll events and block until one is available
    auto res = ::poll(&pfd, 1, -1);
    return res != -1;
}

bool Fifo::has_data() {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    pollfd pfd{
            .fd = fd_,
            .events = POLLIN,
            .revents = 0
    };

    // Poll events and block for 1ms
    auto res = ::poll(&pfd, 1, 1);
    return res > 0;
}

bool Fifo::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if pipe is open
    if (fd_ == -1)
        return false;

    // Serialize body
    std::uint32_t size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_, header_size);

    // Write data into pipe
    auto res = ::write(fd_, buffer_, header_size + size);
    return res != -1;
}

std::tuple<DataHeader, std::unique_ptr<IDataObject>> Fifo::read() {
    constexpr auto header_size = sizeof(DataHeader);
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if pipe is open
    if (fd_ == -1)
        return std::make_tuple(
                DataHeader(0, DataType::INVALID, 0, 0),
                std::unique_ptr<IDataObject>()
        );

    // Read data from pipe
    auto res = ::read(fd_, buffer_, BUFFER_SIZE);
    if (res == -1 || res == 0)
        return std::make_tuple(
                DataHeader(0, DataType::INVALID, 0, 0),
                std::unique_ptr<IDataObject>()
        );

    // Deserialize header
    auto header = DataHeader::deserialize(buffer_, BUFFER_SIZE);

    // Handle each type differently
    switch (header.get_type()) {
        case INVALID:
            break;

        case JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto obj = JavaSymbol::deserialize(&buffer_[header_size], BUFFER_SIZE - header_size);
            return std::make_tuple(
                    header,
                    std::make_unique<JavaSymbol>(obj)
            );
        }
    }

    // Unknown or invalid type
    return std::make_tuple(
            DataHeader(0, DataType::INVALID, 0, 0),
            std::unique_ptr<IDataObject>()
    );
}

/**
 * Get the current timestamp since epoch in nanoseconds.
 *
 * @return Timestamp in nanoseconds.
 */
std::int64_t get_timestamp() {
    const auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(time)
            .time_since_epoch().count();
}

}
