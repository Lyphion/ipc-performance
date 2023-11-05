#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../include/data_header.hpp"
#include "../include/java_symbol.hpp"
#include "../include/datagram_socket.hpp"
#include "../include/fifo.hpp"

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

int main(int argc, char *argv[]) {
    if (argc < 3)
        return EXIT_FAILURE;

    std::string path(argv[1]);
    auto readonly = strtol(argv[2], nullptr, 10) != 0;

    ipc::DatagramSocket socket(path, readonly);
    auto res = socket.open();
    if (!res) {
        std::cout << "Error opening socket" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Socket opened" << std::endl;

    while (true) {
        if (readonly) {
            auto option = socket.read();
            if (!option) {
                std::cout << "Error while reading data" << std::endl;
                continue;
            }

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

            res = socket.write(data);
            if (!res) {
                std::cout << "Error while writing data" << std::endl;
            }

            std::cout << "JavaSymbol" << data << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return EXIT_SUCCESS;
}
