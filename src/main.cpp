#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "../include/datagram_socket.hpp"
#include "../include/fifo.hpp"
#include "../include/message_queue.hpp"
#include "../include/stream_socket.hpp"
#include "../include/dbus.hpp"
#include "../include/shared_memory.hpp"
#include "../include/shared_file.hpp"

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static volatile bool stop = false;

/**
 * Create a new handler by its type.
 *
 * @param type   Type of the handler.
 * @param path   Path or name where to save/handle the data structure.
 * @param reader Whether the handler is targeted for reading/managing or writing.
 *
 * @return Pointer to handler.
 */
std::shared_ptr<ipc::ICommunicationHandler> create_handler(std::string type, const std::string &path, bool reader) {
    if (type == "dbus") {
        return std::make_shared<ipc::DBus>("ipc." + path + ".server", reader);
    } else if (type == "fifo") {
        return std::make_shared<ipc::Fifo>("/tmp/" + path, reader);
    } else if (type == "queue") {
        return std::make_shared<ipc::MessageQueue>("/" + path, reader);
    } else if (type == "dgram") {
        return std::make_shared<ipc::DatagramSocket>("/tmp/" + path, reader);
    } else if (type == "stream") {
        return std::make_shared<ipc::StreamSocket>("/tmp/" + path, reader);
    } else if (type == "udp") {
        return std::make_shared<ipc::DatagramSocket>(path, 8080, reader);
    } else if (type == "tcp") {
        return std::make_shared<ipc::StreamSocket>(path, 8080, reader);
    } else if (type == "memory") {
        return std::make_shared<ipc::SharedMemory>(path, reader, false);
    } else if (type == "mapped") {
        return std::make_shared<ipc::SharedMemory>("/tmp/" + path, reader, true);
    } else if (type == "file") {
        return std::make_shared<ipc::SharedFile>("/tmp/" + path, reader);
    }

    return nullptr;
}

void run_client(const std::shared_ptr<ipc::ICommunicationHandler> &handler) {
    int i = 1;
    while (!stop) {
        ipc::JavaSymbol data(
                static_cast<std::int64_t>(rand()) << 32 | rand(),
                rand(),
                "test123"
        );

        auto res = handler->write(data);
        if (!res) {
            std::cout << "Error while writing data" << std::endl;
        }

        std::cout << "JavaSymbol" << data << " - " << i << std::endl;
        i++;

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2));
    }
}

void run_server(const std::shared_ptr<ipc::ICommunicationHandler> &handler) {
    int i = 0;
    bool more_data = false;

    while (!stop) {
        if (!more_data) {
            std::cout << "Await new data..." << std::endl;
            if (!handler->await_data())
                continue;
        }

        auto result = handler->read();
        more_data = !std::holds_alternative<ipc::CommunicationError>(result);

        std::visit(overloaded{
                [&handler](const ipc::CommunicationError &error) {
                    if (error == ipc::CommunicationError::NO_DATA_AVAILABLE)
                        return;

                    std::cout << "Error while reading data (Error: " << static_cast<int>(error) << ')' << std::endl;

                    if (error == ipc::CommunicationError::CONNECTION_CLOSED) {
                        if (auto s = dynamic_cast<ipc::StreamSocket *>(handler.get())) {
                            s->accept();
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
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Missing arguments" << std::endl;
        return EXIT_FAILURE;
    }

    std::string type(argv[1]);
    std::string path(argv[2]);
    auto readonly = strcmp(argv[3], "reader") == 0;

    auto handler = create_handler(type, path, readonly);
    if (!handler) {
        std::cout << "Invalid parameter" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Loading handler... (" << type << ')' << std::endl;

    auto res = handler->open();
    if (!res) {
        std::cout << "Error opening handler" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Handler opened " << "(readonly=" << readonly << ')' << std::endl;

    std::thread t;
    if (readonly) {
        t = std::thread(run_server, handler);
    } else {
        t = std::thread(run_client, handler);
    }

    std::cin.get();

    stop = true;
    t.join();

    return EXIT_SUCCESS;
}
