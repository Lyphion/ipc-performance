#pragma once

extern "C" {
#include <semaphore.h>
}

#include "communication_handler.hpp"

namespace ipc {

class SharedMemory : public ICommunicationHandler {
public:
    /// Size of the buffer and limit of header and body combined.
    static constexpr short BUFFER_SIZE = 512;

    /// Total amount slots in the buffer.
    static constexpr short TOTAL_AMOUNT = 64;

    /// Total amount of memory to use.
    static constexpr int TOTAL_SIZE = BUFFER_SIZE * TOTAL_AMOUNT * sizeof(std::byte);

    /**
     * Create a new shared memory handler.
     *
     * @param name Name area or file.
     * @param file Whether the name is a path.
     */
    SharedMemory(std::string name, bool server, bool file = false);

    /**
     * Destructor for this object to cleanup data and close memory.
     */
    ~SharedMemory() override;

    bool open() override;

    bool close() override;

    bool await_data() const override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

private:
    const std::string name_;
    const bool server_;
    const bool file_;
    int fd_ = -1;
    int offset_ = 0;
    std::byte *address_ = nullptr;

    sem_t *reader_ = nullptr;
    sem_t *writer_ = nullptr;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
