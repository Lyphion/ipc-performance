#include <chrono>
#include <iostream>
#include <iomanip>

#include "../include/data_header.hpp"
#include "../include/java_symbol.hpp"

/**
 * Print byte array tp console.
 *
 * @param data Byte array to print.
 * @param size Size of array.
 */
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

int main() {
    const auto time = std::chrono::high_resolution_clock::now();
    const std::int64_t timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(time)
            .time_since_epoch().count();

    ipc::DataHeader header(
            1,
            ipc::DataType::JAVA_SYMBOL_LOOKUP,
            0x1234,
            timestamp
    );

    constexpr auto buffer_size = 32;
    auto buffer = new char[buffer_size];

    auto size = header.serialize(buffer, buffer_size);
    std::cout << "Size: " << size << std::endl;
    std::cout << "Header: " << header << std::endl;

    print_array(buffer, buffer_size);

    auto parsed_header = ipc::DataHeader::deserialize(buffer, buffer_size);
    std::cout << "Parsed: " << parsed_header << std::endl;

    std::fill_n(buffer, buffer_size, 0);

    ipc::JavaSymbol symbol(0x7c923f9f4e72ab41, 0x7a, "ABC");
    size = symbol.serialize(buffer, buffer_size);

    std::cout << "Size: " << size << std::endl;
    std::cout << "Symbol: " << symbol << std::endl;

    print_array(buffer, buffer_size);

    auto parsed_symbol = ipc::JavaSymbol::deserialize(buffer, buffer_size);

    delete[] buffer;

    return 0;
}
