#pragma once

#include <ostream>
#include <cstdint>
#include "data_type.hpp"

namespace ipc {

/**
 *  Object containing metadata for the actual message.
 */
class DataHeader {
public:
    /**
     * Create a new data header object containing metadata information.
     *
     * @param id        Id of the message.
     * @param type      Type of the message.
     * @param body_size Size of the actual message.
     * @param timestamp Timestamp when the message/header was created.
     */
    DataHeader(std::uint32_t id, DataType type, std::uint16_t body_size, std::int64_t timestamp)
            : id_(id), type_(type), body_size_(body_size), timestamp_(timestamp) {}

    /**
     * Serialize the header into a buffer.
     *
     * @param buffer Buffer to serialize the header into.
     * @param size   Size of the buffer.
     *
     * @return Total number of bytes written into the buffer.
     */
    unsigned int serialize(char *buffer, unsigned int size) const;

    /**
     * Check if this header is valid.
     *
     * @return True, if header is valid.
     */
    constexpr bool is_valid() const { return type_ != DataType::INVALID; }

    /**
     * Id of the message.
     */
    std::uint32_t get_id() const { return id_; }

    /**
     * Type of the message.
     */
    DataType get_type() const { return type_; }

    /**
     * Size of the actual message.
     */
    std::uint16_t get_body_size() const { return body_size_; }

    /**
     * Timestamp when the message/header was created.
     */
    std::int64_t get_timestamp() const { return timestamp_; }

    /**
     * Deserialize the header from a buffer.
     *
     * @param buffer Buffer to deserialize the header from.
     * @param size   Size of the buffer.
     *
     * @return Deserialize header from buffer.
     */
    static DataHeader deserialize(const char *buffer, unsigned int size);

private:
    std::uint32_t id_;
    DataType type_;
    std::uint16_t body_size_;
    std::int64_t timestamp_;
};

std::ostream &operator<<(std::ostream &outs, const DataHeader &header);

}
