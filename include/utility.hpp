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
