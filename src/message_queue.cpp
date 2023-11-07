#include "../include/message_queue.hpp"

extern "C" {
#include <fcntl.h>
#include <mqueue.h>
#include <sys/poll.h>
#include <sys/stat.h>
}

#include "../include/utility.hpp"

namespace ipc {

MessageQueue::MessageQueue(std::string path, bool readonly)
        : path_(std::move(path)), readonly_(readonly) {}

MessageQueue::~MessageQueue() {
    if (mqd_ != -1) {
        close();
    }
}

bool MessageQueue::open() {
    // Check if message queue is already open
    if (mqd_ != -1)
        return true;

    // Configure block size
    mq_attr attr{};
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_maxmsg = 10;

    // Create and open message queue
    auto flag = readonly_ ? (O_RDWR | O_CREAT | O_NONBLOCK) : (O_RDWR | O_CREAT);
    mqd_ = mq_open(path_.c_str(), flag, 0666, &attr);
    if (mqd_ == -1) {
        mq_unlink(path_.c_str());
        perror("MessageQueue::open (mq_open)");
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
    mq_unlink(path_.c_str());
    mqd_ = -1;

    return true;
}

bool MessageQueue::await_data() const {
    // Check if pipe is open
    if (mqd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = mqd_;
    pfd.events = POLLIN;

    // Poll events and block until one is available
    auto res = ::poll(&pfd, 1, -1);
    return res != -1;
}

bool MessageQueue::has_data() const {
    // Check if pipe is open
    if (mqd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = mqd_;
    pfd.events = POLLIN;

    // Poll events and block for 1ms
    auto res = ::poll(&pfd, 1, 1);
    return res > 0;
}

bool MessageQueue::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if message queue is open
    if (mqd_ == -1)
        return false;

    // Serialize body
    std::uint32_t size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into message queue
    auto res = mq_send(mqd_, reinterpret_cast<const char *>(buffer_.data()), header_size + size, 0);
    return res != -1;
}

std::vector<std::tuple<DataHeader, DataObject>> MessageQueue::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if message queue is open
    if (mqd_ == -1)
        return {};

    std::vector<std::tuple<DataHeader, DataObject>> objects{};

    // Read data from message queue
    long result;
    while (true) {
        result = mq_receive(mqd_, reinterpret_cast<char *>(buffer_.data()), BUFFER_SIZE, nullptr);
        if (result <= 0) {
            break;
        }

        // Deserialize header
        auto optional = DataHeader::deserialize(buffer_.data(), result);
        if (!optional)
            continue;

        auto header = *optional;

        // Handle each type differently
        switch (header.get_type()) {
            case DataType::INVALID:
                continue;

            case DataType::JAVA_SYMBOL_LOOKUP: {
                // Deserialize Java Symbols
                auto data = JavaSymbol::deserialize(&buffer_[header_size], result - header_size);
                if (!data)
                    continue;

                objects.emplace_back(header, *data);
            }
        }
    }

    return objects;
}

}
