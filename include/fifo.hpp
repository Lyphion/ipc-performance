#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "communication_handler.hpp"

namespace ipc {

class Fifo : public ICommunicationHandler {
public:
    /// Size of the buffer and limit of header and body combined.
    static constexpr short BUFFER_SIZE = 512;

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
    ~Fifo();

    /**
     * Open a new pipe.
     *
     * @return True, if pipe was successfully opened.
     */
    bool open();

    /**
     * Close the current pipe.
     *
     * @return True, if pipe was successfully closed.
     */
    bool close();

    /**
     * Poll new data from the pipe.
     *
     * @return True, if poll was successful.
     * @remark Method will block until an event occurred.
     */
    bool await_data() const;

    /**
     * Check if new data is available.
     *
     * @return True, if data is available.
     */
    bool has_data() const;

    bool write(const IDataObject &obj) override;

    std::vector<std::tuple<DataHeader, DataObject>> read() override;

    /**
     * Path of the pipe.
     */
    const std::string &path() const { return path_; }

private:
    const std::string path_;
    const bool readonly_;
    int fd_ = -1;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
