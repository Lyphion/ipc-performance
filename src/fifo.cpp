#include "../include/fifo.hpp"

extern "C" {
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "../include/utility.hpp"

namespace ipc {

Fifo::Fifo(std::string path) : path_(std::move(path)) {}

Fifo::~Fifo() {
    // Close pipe if open
    if (fd_ != -1) {
        close();
    }
}

bool Fifo::open() {
    // Check if pipe is already open
    if (fd_ != -1)
        return true;

    // Create pipe
    int res = mkfifo(path_.c_str(), 0666);
    if (!res)
        return false;

    // Open pipe
    fd_ = ::open(path_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ == -1) {
        unlink(path_.c_str());
        return false;
    }

    return true;
}

bool Fifo::close() {
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

    // Check if pipe is open
    if (fd_ == -1)
        return false;

    // Serialize body
    std::uint32_t size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into pipe
    auto res = ::write(fd_, buffer_.data(), header_size + size);
    return res != -1;
}

std::optional<std::tuple<DataHeader, DataObject>> Fifo::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if pipe is open
    if (fd_ == -1)
        return std::nullopt;

    // Read data from pipe
    auto res = ::read(fd_, buffer_.data(), BUFFER_SIZE);
    if (res == -1 || res == 0)
        return std::nullopt;

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), BUFFER_SIZE);
    if (!optional)
        return std::nullopt;

    auto header = *optional;

    // Handle each type differently
    switch (header.get_type()) {
        case DataType::INVALID:
            break;

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto data = JavaSymbol::deserialize(&buffer_[header_size], BUFFER_SIZE - header_size);
            if (!data)
                return std::nullopt;

            return std::make_tuple(header, *data);
        }
    }

    // Unknown or invalid type
    return std::nullopt;
}

}
