#include "../include/fifo.hpp"

#include <cassert>

extern "C" {
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "../include/utility.hpp"

namespace ipc {

Fifo::Fifo(std::string path, bool readonly)
        : path_(std::move(path)), readonly_(readonly) {}

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
    if (!res) {
        perror("Fifo::open (mkfifo)");
        return false;
    }

    // Open pipe
    auto flag = readonly_ ? O_RDWR | O_NONBLOCK : O_RDWR;
    fd_ = ::open(path_.c_str(), flag);
    if (fd_ == -1) {
        perror("Fifo::open (open)");
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
    ::close(fd_);
    unlink(path_.c_str());
    fd_ = -1;

    return true;
}

bool Fifo::await_data() const {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    // Poll events and block until one is available
    auto res = ::poll(&pfd, 1, -1);
    return res != -1;
}

bool Fifo::has_data() const {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

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
    auto result = ::read(fd_, buffer_.data(), header_size);
    if (result <= 0)
        return std::nullopt;

    assert(header_size == result);

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), result);
    if (!optional)
        return std::nullopt;

    auto header = *optional;

    result = ::read(fd_, buffer_.data(), header.get_body_size());
    if (result <= 0)
        return std::nullopt;

    assert(header.get_body_size() == result);

    // Handle each type differently
    switch (header.get_type()) {
        case DataType::INVALID:
            break;

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto data = JavaSymbol::deserialize(buffer_.data(), result);
            if (!data)
                return std::nullopt;

            return std::make_tuple(header, *data);
        }
    }

    return std::nullopt;
}

}
