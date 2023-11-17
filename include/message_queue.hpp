#pragma once

#include "communication_handler.hpp"

namespace ipc {

class MessageQueue : public ICommunicationHandler {
public:
    /// Amount of elements in the queue.
    static constexpr short QUEUE_SIZE = 10;

    /**
     * Create a new message queue.
     *
     * @param path     Path to the message queue.
     * @param readonly Whether the message queue is for read only.
     */
    MessageQueue(std::string path, bool readonly);

    /**
     * Destructor for this object to cleanup data and close message queue.
     */
    ~MessageQueue() override;

    bool open() override;

    bool close() override;

    bool is_open() const override;

    bool await_data() override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

    /**
     * Path of the message queue.
     */
    const std::string &path() const { return path_; }

    /**
     * Whether is message queue is targeted for read only.
     */
    bool readonly() const { return readonly_; }

private:
    const std::string path_;
    const bool readonly_;
    int mqd_ = -1;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
