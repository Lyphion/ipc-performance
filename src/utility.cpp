#include "../include/utility.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>

void print_array(const char *data, int size) {
    std::ios_base::fmtflags f(std::cout.flags());

    std::cout << std::setfill('0') << std::hex;
    for (auto i = 0; i < size; i++) {
        auto val = static_cast<int>(data[i]) & 0xff;
        std::cout << std::setw(sizeof(char) * 2) << val;
    }

    std::cout << std::endl;
    std::cout.flags(f);
}

std::int64_t get_timestamp() {
    const auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(time)
            .time_since_epoch().count();
}
