#include "../include/fifo.hpp"

#include <cassert>

extern "C" {
#include <fcntl.h>
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
        Fifo::close();
    }
}

bool Fifo::open() {
    // Check if pipe is already open
    if (fd_ != -1)
        return true;

    if (readonly_) {
        // Create pipe
        unlink(path_.c_str());

        const auto res = mkfifo(path_.c_str(), 0660);
        if (res == -1) {
            perror("Fifo::open (mkfifo)");
            return false;
        }
    }

    // Open pipe
    const auto flag = readonly_ ? O_RDWR | O_NONBLOCK : O_RDWR;
    fd_ = ::open(path_.c_str(), flag);
    if (fd_ == -1) {
        perror("Fifo::open (open)");

        if (readonly_)
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

    if (readonly_)
        unlink(path_.c_str());
    fd_ = -1;

    return true;
}

bool Fifo::is_open() const {
    return fd_ != -1;
}

bool Fifo::await_data() {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    // Poll events and block until one is available
    const auto res = poll(fd_, WAIT_TIME);
    if (res == -1)
        perror("Fifo::await_data (poll)");

    return res > 0;
}

bool Fifo::has_data() const {
    // Check if pipe is open
    if (fd_ == -1)
        return false;

    // Poll events and block for 1ms
    const auto res = poll(fd_, 1);
    if (res == -1)
        perror("Fifo::has_data (poll)");

    return res > 0;
}

bool Fifo::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if pipe is open
    if (fd_ == -1)
        return false;

    // Serialize body
    const auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1)
        return false;

    last_id_++;
    const auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into pipe
    const auto res = ::write(fd_, buffer_.data(), header_size + size);

    if (res == -1)
        perror("Fifo::write (write)");

    return res != -1;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> Fifo::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if pipe is open
    if (fd_ == -1)
        return CommunicationError::CONNECTION_CLOSED;

    // Read data from pipe
    auto result = ::read(fd_, buffer_.data(), header_size);
    if (result == -1) {
        if (errno == EAGAIN)
            return CommunicationError::NO_DATA_AVAILABLE;

        perror("Fifo::read (read)");
        return CommunicationError::READ_ERROR;
    }

    if (result == 0)
        return CommunicationError::NO_DATA_AVAILABLE;

    assert(header_size == result);

    // Deserialize header
    const auto optional = DataHeader::deserialize(buffer_.data(), result);
    if (!optional)
        return CommunicationError::INVALID_HEADER;

    const auto header = *optional;

    result = ::read(fd_, buffer_.data(), header.get_body_size());
    if (result == -1) {
        if (errno == EAGAIN)
            return CommunicationError::NO_DATA_AVAILABLE;

        perror("Fifo::read (read)");
        return CommunicationError::READ_ERROR;
    }

    if (result == 0)
        return CommunicationError::NO_DATA_AVAILABLE;

    assert(header.get_body_size() == result);

    const auto body = deserialize_data_object(header.get_type(), buffer_.data(), header.get_body_size());

    if (std::holds_alternative<DataObject>(body)) {
        const auto obj = std::get<DataObject>(body);
        return std::make_tuple(header, obj);
    } else {
        return std::get<CommunicationError>(body);
    }
}

}
