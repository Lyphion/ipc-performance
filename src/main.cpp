#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../include/datagram_socket.hpp"
#include "../include/fifo.hpp"
#include "../include/message_queue.hpp"

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
    auto readonly = strcmp(argv[2], "reader") == 0;

    ipc::MessageQueue handler(path, readonly);
    auto res = handler.open();
    if (!res) {
        std::cout << "Error opening handler" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Handler opened (readonly=" << readonly << ')' << std::endl;

    int i = 1;
    while (true) {
        if (readonly) {
            if (!handler.await_data())
                continue;

            auto option = handler.read();
            if (!option) {
                std::cout << "Error while reading data" << std::endl;
                continue;
            }

            auto [header, data] = *option;

            if (!header.is_valid()) {
                std::cout << "Invalid data received" << std::endl;
                continue;
            }

            std::cout << "Header" << header << " - " << i << std::endl;
            std::visit(overloaded{
                    [](const ipc::JavaSymbol &symbol) {
                        std::cout << "JavaSymbol" << symbol << std::endl;
                    }
            }, data);
            i++;

            if (rand() % 25 == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            ipc::JavaSymbol data(
                    static_cast<std::int64_t>(rand()) << 32 | rand(),
                    rand(),
                    "test123"
            );

            res = handler.write(data);
            if (!res) {
                std::cout << "Error while writing data" << std::endl;
            }

            std::cout << "JavaSymbol" << data << " - " << i << std::endl;
            i++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return EXIT_SUCCESS;
}
