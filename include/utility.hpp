#pragma once

#include <cstdint>

/**
 * Print byte array tp console.
 *
 * @param data Byte array to print.
 * @param size Size of array.
 */
void print_array(const char *data, int size);

/**
 * Get the current timestamp since epoch in nanoseconds.
 *
 * @return Timestamp in nanoseconds.
 */
std::int64_t get_timestamp();
