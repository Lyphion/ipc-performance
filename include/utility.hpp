#pragma once

#include <cstddef>
#include <cstdint>

#include "handler/communication_handler.hpp"

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace ipc {

/**
 * Print byte array to console.
 *
 * @param data Byte array to print.
 * @param size Size of array.
 */
void print_array(const std::byte *data, unsigned int size);

/**
 * Get the current timestamp since epoch in nanoseconds.
 *
 * @return Timestamp in nanoseconds.
 */
std::int64_t get_timestamp();

/**
 * Poll data from file descriptor.
 *
 * @param fd      File descriptor to poll from.
 * @param timeout Time in milliseconds to wait while polling, or -1 for infinite.
 *
 * @return Returns 1 if successful, zero if timed out, or -1 for errors.
 */
int poll(int fd, int timeout);

/**
 * Deserialize a DataObject from a buffer with given size.
 *
 * @param type   Type of the object.
 * @param buffer Buffer with serialized object data.
 * @param size   Size of the buffer.
 *
 * @return Deserialized object or error.
 */
std::variant<DataObject, CommunicationError> deserialize_data_object(
        DataType type, const std::byte *buffer, unsigned int size);

}
