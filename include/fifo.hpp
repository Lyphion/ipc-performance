#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "communication_handler.hpp"

namespace ipc {

class Fifo : public ICommunicationHandler {
public:
    /**
     * Create a new Fifo pipe.
     *
     * @param path     Path to the pipe.
     * @param readonly Whether the pipe is for read only.
     */
    Fifo(std::string path, bool readonly);

    /**
     * Destructor for this object to cleanup data and close pipe.
     */
    ~Fifo() override;

    bool open() override;

    bool close() override;

    bool is_open() const override;

    bool await_data() override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

    /**
     * Path of the pipe.
     */
    const std::string &path() const { return path_; }

    /**
     * Whether is pipe is targeted for read only.
     */
    bool readonly() const { return readonly_; }

private:
    const std::string path_;
    const bool readonly_;
    int fd_ = -1;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
