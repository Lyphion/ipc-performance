#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../include/datagram_socket.hpp"
#include "../include/fifo.hpp"
#include "../include/message_queue.hpp"
#include "../include/stream_socket.hpp"

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename T>
std::string build_path(const std::string &name) {
    if constexpr (std::is_same_v<T, ipc::MessageQueue>) {
        return "/" + name;
    } else {
        return "/tmp/" + name;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3)
        return EXIT_FAILURE;

    using Type = ipc::StreamSocket;

    std::string path(build_path<Type>(argv[1]));
    auto readonly = strcmp(argv[2], "reader") == 0;

    std::cout << "Loading handler... (" << typeid(Type).name() << ')' << std::endl;

    Type handler(path, readonly);
    auto res = handler.open();
    if (!res) {
        std::cout << "Error opening handler" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Handler opened " << "(readonly=" << readonly << ')' << std::endl;

    int i = 1;
    bool more_data = false;
    while (true) {
        if (readonly) {
            if (!more_data) {
                std::cout << "Await new data..." << std::endl;
                if (!handler.await_data())
                    continue;
            }

            auto result = handler.read();
            more_data = !std::holds_alternative<ipc::CommunicationError>(result);

            std::visit(overloaded{
                    [&handler](const ipc::CommunicationError &error) {
                        if (error == ipc::CommunicationError::NO_DATA_AVAILABLE)
                            return;

                        std::cout << "Error while reading data (Error: "
                                  << static_cast<int>(error) << ')' << std::endl;

                        if constexpr (std::is_same_v<Type, ipc::StreamSocket>) {
                            if (error == ipc::CommunicationError::CONNECTION_CLOSED) {
                                auto& s = static_cast<ipc::StreamSocket&>(handler);
                                s.accept();
                                std::cout << "Client reconnected" << std::endl;
                            }
                        }
                    },
                    [&i](const auto &success) {
                        auto [header, data] = success;

                        std::cout << "Header" << header << " - " << i << std::endl;
                        std::visit(overloaded{
                                [](const ipc::JavaSymbol &symbol) {
                                    std::cout << "JavaSymbol" << symbol << std::endl;
                                }
                        }, data);
                        i++;

                        if (i % 25 == 0)
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
            }, result);
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

            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 20));
        }
    }

    return EXIT_SUCCESS;
}
