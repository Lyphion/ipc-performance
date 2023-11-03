#include <chrono>
#include <iostream>
#include <thread>

#include "../include/data_header.hpp"
#include "../include/java_symbol.hpp"
#include "../include/fifo.hpp"

/*
 *  std::variant
 *      std::visit
 *  std::any
 *  std::array statt char*
 *  std::byte
 */

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

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

            auto option = pipe.read();
            if (!option)
                continue;

            auto [header, data] = *option;

            if (!header.is_valid()) {
                std::cout << "Invalid data received" << std::endl;
                continue;
            }

            std::cout << "Header" << header << std::endl;
            std::visit(overloaded{
                    [](ipc::JavaSymbol &symbol) {
                        std::cout << "JavaSymbol" << symbol << std::endl;
                    }
            }, data);
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
