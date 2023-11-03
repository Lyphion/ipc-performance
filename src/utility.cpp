#include "../include/utility.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>

void print_array(const std::byte *data, unsigned int size) {
    std::ios_base::fmtflags f(std::cout.flags());

    std::cout << std::setfill('0') << std::hex;
    for (unsigned int i = 0; i < size; i++) {
        std::cout << std::setw(sizeof(char) * 2) << static_cast<int>(data[i]);
    }

    std::cout << std::endl;
    std::cout.flags(f);
}

std::int64_t get_timestamp() {
    const auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(time)
            .time_since_epoch().count();
}
