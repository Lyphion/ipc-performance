#include "handler/shared_file.hpp"

#include <cassert>
#include <thread>
#include <utility>

extern "C" {
#include <fcntl.h>
}

#include "utility.hpp"

namespace ipc {

SharedFile::SharedFile(std::string path, bool server)
        : path_(std::move(path)), server_(server) {}

SharedFile::~SharedFile() {
    if (file_.is_open()) {
        SharedFile::close();
    }
}

bool SharedFile::open() {
    // Check if file is already open
    if (file_.is_open())
        return true;

    if (server_) {
        file_ = std::fstream(path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    } else {
        file_ = std::fstream(path_, std::ios::in | std::ios::out | std::ios::binary);
    }

    if (!file_.is_open()) {
        perror("SharedFile::open (fstream)");
        return false;
    }

    std::string prefix(path_.substr(path_.rfind('/') + 1) + "_sem");

    if (server_) {
        sem_unlink((prefix + "r").c_str());
        sem_unlink((prefix + "w").c_str());
    }

    const auto flag = server_ ? O_CREAT : 0;
    reader_ = sem_open((prefix + "r").c_str(), flag, 0660, 0);
    if (reader_ == SEM_FAILED) {
        perror("SharedFile::open (sem_open)");
        close();
        return false;
    }

    writer_ = sem_open((prefix + "w").c_str(), flag, 0660, TOTAL_AMOUNT);
    if (writer_ == SEM_FAILED) {
        perror("SharedFile::open (sem_open)");
        close();
        return false;
    }

    return true;
}

bool SharedFile::close() {
    // Check if file is already closed
    if (!file_.is_open())
        return false;

    file_.close();

    sem_close(reader_);
    sem_close(writer_);

    if (server_) {
        remove(path_.c_str());

        std::string prefix(path_.substr(path_.rfind('/') + 1) + "_sem");
        sem_unlink((prefix + "r").c_str());
        sem_unlink((prefix + "w").c_str());
    }

    return true;
}

bool SharedFile::is_open() const {
    return file_.is_open();
}

bool SharedFile::await_data() {
    // Check if file is already closed
    if (!file_.is_open())
        return false;

#if WAIT_TIME == -1
    // Wait for data
    const auto res = sem_wait(reader_);

    if (res == -1) {
        perror("SharedMemory::await_data (sem_wait)");
        return false;
    }
#else
    timespec wait_time{};
    if (clock_gettime(CLOCK_REALTIME, &wait_time) == -1)
        return false;

    wait_time.tv_sec += WAIT_TIME / 1000;
    // Wait for data
    const auto res = sem_timedwait(reader_, &wait_time);

    if (res == -1) {
        if (errno != ETIMEDOUT)
            perror("SharedMemory::await_data (sem_timedwait)");

        return false;
    }
#endif

    // Increment semaphore (because this method just checks if data is available)
    sem_post(reader_);
    return true;
}

bool SharedFile::has_data() const {
    // Check if file is already closed
    if (!file_.is_open())
        return false;

    // Check if data is available
    int value;
    const auto res = sem_getvalue(reader_, &value);
    if (res == -1)
        perror("SharedFile::has_data (sem_getvalue)");

    return res != -1 && value > 0;
}

bool SharedFile::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if file is already closed
    if (!file_.is_open())
        return false;

    // Serialize body
    const auto timestamp = get_timestamp();
    const auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1) {
        sem_post(writer_);
        return false;
    }

    last_id_++;
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Clear error bits
    file_.clear();

    // Move write pointer to correct location
    file_.seekp(offset_ * BUFFER_SIZE, std::ios::beg);
    if (file_.fail()) {
        perror("SharedFile::write (seekp)");
        sem_post(writer_);
        return false;
    }

    // Wait until file is available
    const auto res = sem_wait(writer_);
    if (res == -1) {
        perror("SharedFile::write (sem_wait)");
        return false;
    }

    // Write data
    const auto data = reinterpret_cast<const char *>(buffer_.data());
    file_.write(data, static_cast<long>(header_size) + size);
    if (file_.fail()) {
        perror("SharedFile::write (write)");
        sem_post(writer_);
        return false;
    }

    file_.flush();
    offset_ = (offset_ + 1) % TOTAL_AMOUNT;

    sem_post(reader_);
    return true;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> SharedFile::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if file is open
    if (!file_.is_open())
        return CommunicationError::CONNECTION_CLOSED;

    // Wait and check if data is still available
    const auto res = sem_trywait(reader_);
    if (res == -1) {
        if (errno == EAGAIN) {
            return CommunicationError::NO_DATA_AVAILABLE;
        }

        perror("SharedFile::read (sem_trywait)");
        return CommunicationError::READ_ERROR;
    }

    // Clear error bits
    file_.clear();

    // Move read pointer to correct location
    file_.seekg(offset_ * BUFFER_SIZE, std::ios::beg);
    if (file_.fail()) {
        perror("SharedFile::read (seekg)");
        sem_post(reader_);
        return CommunicationError::READ_ERROR;
    }

    // Read data
    const auto data = reinterpret_cast<char *>(buffer_.data());
    file_.read(data, BUFFER_SIZE);
    if (file_.fail() && !file_.eof()) {
        perror("SharedFile::read (read)");
        sem_post(reader_);
        return CommunicationError::READ_ERROR;
    }

    offset_ = (offset_ + 1) % TOTAL_AMOUNT;
    sem_post(writer_);

    // Deserialize header
    const auto optional = DataHeader::deserialize(buffer_.data(), header_size);
    if (!optional)
        return CommunicationError::INVALID_HEADER;

    const auto header = *optional;
    assert(header.get_body_size() <= BUFFER_SIZE - header_size);

    const auto body = deserialize_data_object(header.get_type(), &buffer_[header_size], header.get_body_size());

    if (std::holds_alternative<DataObject>(body)) {
        const auto obj = std::get<DataObject>(body);
        return std::make_tuple(header, obj);
    } else {
        return std::get<CommunicationError>(body);
    }
}

}
