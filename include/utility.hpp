#pragma once

#include <cstddef>
#include <cstdint>

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
