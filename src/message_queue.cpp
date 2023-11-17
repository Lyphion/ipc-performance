#include "../include/message_queue.hpp"

#include <cassert>

extern "C" {
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
}

#include "../include/utility.hpp"

namespace ipc {

MessageQueue::MessageQueue(std::string path, bool readonly)
        : path_(std::move(path)), readonly_(readonly) {}

MessageQueue::~MessageQueue() {
    if (mqd_ != -1) {
        MessageQueue::close();
    }
}

bool MessageQueue::open() {
    // Check if message queue is already open
    if (mqd_ != -1)
        return true;

    // Clear old messages
    if (readonly_)
        mq_unlink(path_.c_str());

    // Configure block size
    mq_attr attr{};
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_maxmsg = QUEUE_SIZE;

    // Create and open message queue
    auto flag = readonly_ ? O_RDWR | O_CREAT | O_NONBLOCK : O_RDWR | O_CREAT;
    mqd_ = mq_open(path_.c_str(), flag, 0660, &attr);
    if (mqd_ == -1) {
        perror("MessageQueue::open (mq_open)");

        if (readonly_)
            mq_unlink(path_.c_str());
        return false;
    }

    return true;
}

bool MessageQueue::close() {
    // Check if message queue is already closed
    if (mqd_ == -1)
        return false;

    // Close and unlink message queue
    mq_close(mqd_);
    if (readonly_)
        mq_unlink(path_.c_str());
    mqd_ = -1;

    return true;
}

bool MessageQueue::is_open() const {
    return mqd_ != -1;
}

bool MessageQueue::await_data() {
    // Check if message queue is open
    if (mqd_ == -1)
        return false;

    // Poll events and block until one is available
    auto res = poll(mqd_, WAIT_TIME);
    if (res == -1)
        perror("MessageQueue::await_data (poll)");

    return res > 0;
}

bool MessageQueue::has_data() const {
    // Check if message queue is open
    if (mqd_ == -1)
        return false;

    // Poll events and block for 1ms
    auto res = poll(mqd_, 1);
    if (res == -1)
        perror("MessageQueue::has_data (poll)");

    return res > 0;
}

bool MessageQueue::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if message queue is open
    if (mqd_ == -1)
        return false;

    // Serialize body
    auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1)
        return false;

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into message queue
    auto res = mq_send(mqd_, reinterpret_cast<const char *>(buffer_.data()), header_size + size, 0);
    if (res == -1)
        perror("MessageQueue::write (mq_send)");

    return res != -1;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> MessageQueue::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if message queue is open
    if (mqd_ == -1)
        return CommunicationError::CONNECTION_CLOSED;

    // Read data from message queue
    auto result = mq_receive(mqd_, reinterpret_cast<char *>(buffer_.data()), BUFFER_SIZE, nullptr);
    if (result == -1) {
        if (errno == EAGAIN)
            return CommunicationError::NO_DATA_AVAILABLE;

        perror("MessageQueue::read (mq_receive)");
        return CommunicationError::READ_ERROR;
    }

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), result);
    if (!optional)
        return CommunicationError::INVALID_HEADER;

    auto header = *optional;
    assert(header.get_body_size() == result - header_size);

    auto body = deserialize_data_object(header.get_type(), &buffer_[header_size], header.get_body_size());

    if (std::holds_alternative<DataObject>(body)) {
        auto obj = std::get<DataObject>(body);
        return std::make_tuple(header, obj);
    } else {
        return std::get<CommunicationError>(body);
    }
}

}
