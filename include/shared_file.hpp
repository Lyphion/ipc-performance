#pragma once

#include <fstream>

extern "C" {
#include <semaphore.h>
}

#include "communication_handler.hpp"

namespace ipc {

class SharedFile : public ICommunicationHandler {
public:
    /// Total amount slots in the buffer.
    static constexpr short TOTAL_AMOUNT = 64;

    /// Total amount of memory to use.
    static constexpr int TOTAL_SIZE = BUFFER_SIZE * TOTAL_AMOUNT * sizeof(std::byte);

    /**
     * Create a new shared file handler.
     *
     * @param path   Path for the file.
     * @param server Whether is object manages the file.
     */
    SharedFile(std::string path, bool server);

    /**
     * Destructor for this object to cleanup data and close file.
     */
    ~SharedFile() override;

    bool open() override;

    bool close() override;

    bool is_open() const override;

    bool await_data() override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

private:
    const std::string path_;
    const bool server_;
    std::fstream file_;
    int offset_ = 0;

    sem_t *reader_ = nullptr;
    sem_t *writer_ = nullptr;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
