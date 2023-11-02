#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>

#include "../include/data_header.hpp"
#include "../include/java_symbol.hpp"
#include "../include/fifo.hpp"

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

int main(int argc, char *argv[]) {
    /*
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

     */

    if (argc < 3)
        return 1;

    std::string path(argv[1]);
    bool readonly = strtol(argv[2], nullptr, 10) != 0;

    ipc::Fifo pipe(path);
    bool res = pipe.open();
    if (!res) {
        std::cout << "Error opening pipe" << std::endl;
        return -1;
    }

    std::cout << "Pipe opened" << std::endl;

    while (true) {
        if (readonly) {
            if (!pipe.await_data())
                continue;

            auto pair = pipe.read();
            auto header = std::get<0>(pair);

            if (!header.is_valid()) {
                std::cout << "Invalid data received" << std::endl;
            } else {
                std::cout << "Header" << header << std::endl;
                switch (header.get_type()) {
                    case ipc::INVALID:
                        break;
                    case ipc::JAVA_SYMBOL_LOOKUP:
                        auto symbol = dynamic_cast<ipc::JavaSymbol *>(std::get<1>(pair).get());
                        std::cout << "JavaSymbol" << *symbol << std::endl;
                        break;
                }
            }
        } else {
            ipc::JavaSymbol data(
                    static_cast<std::int64_t>(rand()) << 32 | rand(),
                    rand(),
                    "test123"
            );

            pipe.write(data);
            std::cout << "JavaSymbol" << data << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
