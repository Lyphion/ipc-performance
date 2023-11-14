#include "shared_memory.hpp"

#include <cstring>

extern "C" {
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "utility.hpp"

namespace ipc {

SharedMemory::SharedMemory(std::string name, bool server, bool file)
        : name_(std::move(name)), server_(server), file_(file) {}

SharedMemory::~SharedMemory() {
    if (fd_ != -1) {
        SharedMemory::close();
    }
}

bool SharedMemory::open() {
    // Check if memory is already open
    if (fd_ != -1)
        return true;

    if (file_) {
        if (server_) {
            // Create memory
            remove(name_.c_str());
            fd_ = ::open(name_.c_str(), O_RDWR | O_CREAT, 0660);
        } else {
            // Open memory
            fd_ = ::open(name_.c_str(), O_RDWR);
        }

        if (fd_ == -1) {
            perror("SharedMemory::open (open)");
            return false;
        }
    } else {
        if (server_) {
            // Create memory
            shm_unlink(name_.c_str());
            fd_ = shm_open(name_.c_str(), O_RDWR | O_CREAT, 0660);
        } else {
            // Open memory
            fd_ = shm_open(name_.c_str(), O_RDWR, 0660);
        }

        if (fd_ == -1) {
            perror("SharedMemory::open (shm_open)");
            return false;
        }
    }

    if (server_) {
        // Resize memory
        if (ftruncate(fd_, TOTAL_SIZE) == -1) {
            perror("SharedMemory::open (ftruncate)");
            close();
            return false;
        }
    }

    // Allocate memory
    auto addr = mmap(nullptr, TOTAL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (addr == MAP_FAILED) {
        perror("SharedMemory::open (mmap)");
        close();
        return false;
    }

    address_ = static_cast<std::byte *>(addr);

    std::string prefix(name_.substr(name_.rfind('/') + 1) + "_sem");

    if (server_) {
        sem_unlink((prefix + "r").c_str());
        sem_unlink((prefix + "w").c_str());
    }

    auto flag = server_ ? O_CREAT : 0;
    reader_ = sem_open((prefix + "r").c_str(), flag, 0660, 0);
    if (reader_ == SEM_FAILED) {
        perror("SharedMemory::open (sem_open)");
        close();
        return false;
    }

    writer_ = sem_open((prefix + "w").c_str(), flag, 0660, TOTAL_AMOUNT);
    if (writer_ == SEM_FAILED) {
        perror("SharedMemory::open (sem_open)");
        close();
        return false;
    }

    return true;
}

bool SharedMemory::close() {
    // Check if memory is already closed
    if (fd_ == -1)
        return false;

    munmap(address_, TOTAL_SIZE);
    address_ = nullptr;

    ::close(fd_);
    fd_ = -1;

    sem_close(reader_);
    sem_close(writer_);

    if (server_) {
        if (file_) {
            remove(name_.c_str());
        } else {
            shm_unlink(name_.c_str());
        }

        std::string prefix(name_.substr(name_.rfind('/') + 1) + "_sem");
        sem_unlink((prefix + "r").c_str());
        sem_unlink((prefix + "w").c_str());
    }

    return true;
}

bool SharedMemory::await_data() const {
    // Check if memory is already closed
    if (fd_ == -1)
        return false;

    // Wait for data
    auto res = sem_wait(reader_);
    if (res == -1) {
        perror("SharedMemory::await_data (sem_wait)");
        return false;
    }

    // Increment semaphore (because this method just checks if data is available)
    sem_post(reader_);
    return true;
}

bool SharedMemory::has_data() const {
    // Check if memory is already closed
    if (fd_ == -1)
        return false;

    // Check if data is available
    int value;
    auto res = sem_getvalue(reader_, &value);
    if (res == -1)
        perror("SharedMemory::has_data (sem_getvalue)");

    return res != -1 && value > 0;
}

bool SharedMemory::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if memory is already closed
    if (fd_ == -1)
        return false;

    // Wait until memory is available
    auto res = sem_wait(writer_);
    if (res == -1) {
        perror("SharedMemory::write (sem_wait)");
        return false;
    }

    // Serialize body
    auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1) {
        sem_post(writer_);
        return false;
    }

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Copy data to memory
    std::memcpy(&address_[offset_ * BUFFER_SIZE], buffer_.data(), BUFFER_SIZE);
    offset_ = (offset_ + 1) % TOTAL_AMOUNT;

    sem_post(reader_);
    return true;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> SharedMemory::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if memory is open
    if (fd_ == -1)
        return CommunicationError::CONNECTION_CLOSED;

    // Wait and check if data is still available
    timespec wait_time{};
    wait_time.tv_nsec = 100;

    auto res = sem_timedwait(reader_, &wait_time);
    if (res == -1) {
        if (errno == ETIMEDOUT) {
            return CommunicationError::NO_DATA_AVAILABLE;
        }

        perror("SharedMemory::read (sem_timedwait)");
        return CommunicationError::READ_ERROR;
    }

    // Copy data from memory
    std::memcpy(buffer_.data(), &address_[offset_ * BUFFER_SIZE], BUFFER_SIZE);
    offset_ = (offset_ + 1) % TOTAL_AMOUNT;

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), BUFFER_SIZE);
    if (!optional) {
        sem_post(writer_);
        return CommunicationError::INVALID_HEADER;
    }

    auto header = *optional;

    // Handle each type differently
    switch (header.get_type()) {
        case DataType::INVALID: {
            sem_post(writer_);
            return CommunicationError::INVALID_DATA;
        }

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto data = JavaSymbol::deserialize(&buffer_[header_size], BUFFER_SIZE - header_size);
            sem_post(writer_);

            if (!data)
                return CommunicationError::INVALID_DATA;

            return std::make_tuple(header, *data);
        }
    }

    // Unknown or invalid type
    sem_post(writer_);
    return CommunicationError::UNKNOWN_DATA;
}


}
