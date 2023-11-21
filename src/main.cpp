#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>

#include "benchmark/execution.hpp"
#include "benchmark/latency.hpp"
#include "benchmark/realworld.hpp"
#include "benchmark/throughput.hpp"
#include "handler/datagram_socket.hpp"
#include "handler/dbus.hpp"
#include "handler/fifo.hpp"
#include "handler/message_queue.hpp"
#include "handler/shared_file.hpp"
#include "handler/shared_memory.hpp"
#include "handler/stream_socket.hpp"
#include "object/binary_data.hpp"
#include "utility.hpp"

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
std::shared_ptr<ipc::ICommunicationHandler> create_handler(const std::string &type, const std::string &path, bool reader) {
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
    while (!stop && handler->is_open()) {
        const auto ts = ipc::get_timestamp();
        ipc::JavaSymbol data(
                static_cast<std::int64_t>(rand()) << 32 | rand(),
                rand(),
                "test123"
        );

        const auto res = handler->write(data);
        if (!res) {
            std::cout << "Error while writing data" << std::endl;
        }

        std::cout << ts << " JavaSymbol" << data << " - " << i << std::endl;
        i++;

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2));
    }
}

void run_server(const std::shared_ptr<ipc::ICommunicationHandler> &handler) {
    int i = 1;
    bool more_data = false;

    while (!stop && handler->is_open()) {
        if (!more_data) {
            std::cout << "Await new data..." << std::endl;
            if (!handler->await_data())
                continue;
        }

        const auto result = handler->read();
        more_data = !std::holds_alternative<ipc::CommunicationError>(result);

        std::visit(overloaded{
                [&handler](const ipc::CommunicationError &error) {
                    // 'No data available' is not a real error, so ignore it
                    if (error == ipc::CommunicationError::NO_DATA_AVAILABLE)
                        return;

                    std::cout << "Error while reading data (Error: " << static_cast<int>(error) << ')' << std::endl;

                    // Clear old connection and try to find new client if connection closed and handler is stream socket
                    if (error == ipc::CommunicationError::CONNECTION_CLOSED) {
                        if (const auto s = dynamic_cast<ipc::StreamSocket *>(handler.get())) {
                            if (s->accept()) {
                                std::cout << "Client reconnected" << std::endl;
                            }
                        }
                    }
                },
                [&i](const auto &success) {
                    auto [header, data] = success;

                    std::cout << "Header" << header << " - " << i << std::endl;
                    std::visit(overloaded{
                            [](const ipc::JavaSymbol &symbol) {
                                std::cout << "JavaSymbol" << symbol << std::endl;
                            },
                            [](const ipc::Ping &ping) {
                                std::cout << "Ping" << ping << std::endl;
                            },
                            [](const ipc::BinaryData &binary) {
                                std::cout << "BinaryData" << binary << std::endl;
                            }
                    }, data);
                    i++;

                    //if (i % 25 == 0)
                    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
        }, result);
    }
}

int run_latency(ipc::ICommunicationHandler &handler, bool readonly) {
    constexpr auto iterations = 1000;
    ipc::benchmark::LatencyBenchmark bench(iterations, 10, readonly);
    if (!bench.setup(handler))
        return EXIT_FAILURE;

    std::cout << "Running Latency benchmark..." << std::endl;
    const auto success = bench.run(handler);
    std::cout << "Benchmark completed!" << std::endl;

    bench.cleanup(handler);

    if (!success)
        return EXIT_FAILURE;

    if (readonly) {
        const auto res = bench.get_results();
        const auto count = res.size();
        const auto [min, max] = std::minmax_element(res.begin(), res.end());
        const auto sum = std::reduce(res.begin(), res.end());
        const auto avg = static_cast<double>(sum) / static_cast<double>(count);

        std::cout << "Iterations: " << count << std::endl
                  << "Minimum:    " << *min / 1000.0 << "us" << std::endl
                  << "Maximum:    " << *max / 1000.0 << "us" << std::endl
                  << "Average:    " << avg / 1000.0 << "us" << std::endl;
#if 0
        std::cout << "Data: ";
        for (auto &i: bench.get_results())
            std::cout << i << ' ';
        std::cout << std::endl;
#endif
    }

    return EXIT_SUCCESS;
}

int run_throughput(ipc::ICommunicationHandler &handler, bool readonly) {
    constexpr auto iterations = 1'000'000;
    ipc::benchmark::ThroughputBenchmark bench(iterations, 512 - sizeof(ipc::DataHeader), readonly);
    if (!bench.setup(handler))
        return EXIT_FAILURE;

    std::cout << "Running Throughput benchmark..." << std::endl;
    const auto success = bench.run(handler);
    std::cout << "Benchmark completed!" << std::endl;

    bench.cleanup(handler);

    if (!success)
        return EXIT_FAILURE;

    if (readonly) {
        const auto count = bench.get_iterations();
        const auto received = bench.get_received();
        const auto size = bench.get_size();
        const auto total_time = bench.get_total_time();
        const auto throughput = bench.get_throughput();

        std::cout << "Iterations: " << count << std::endl
                  << "Size:       " << size << " Byte (" << size + sizeof(ipc::DataHeader) << " Byte)" << std::endl
                  << "Misses:     " << count - received << std::endl
                  << "Time:       " << total_time << "ms" << std::endl
                  << "Throughput: " << throughput << "KiB/s" << std::endl;
    }

    return EXIT_SUCCESS;
}

int run_execution_time(ipc::ICommunicationHandler &handler, bool readonly) {
    constexpr auto iterations = 1000;
    ipc::benchmark::ExecutionTimeBenchmark bench(iterations, 512 - sizeof(ipc::DataHeader), 10, readonly);
    if (!bench.setup(handler))
        return EXIT_FAILURE;

    std::cout << "Running Execution Time benchmark..." << std::endl;
    const auto success = bench.run(handler);
    std::cout << "Benchmark completed!" << std::endl;

    bench.cleanup(handler);

    if (!success)
        return EXIT_FAILURE;

    const auto res = bench.get_results();
    const auto count = res.size();
    const auto size = bench.get_size();
    const auto [min, max] = std::minmax_element(res.begin(), res.end());
    const auto sum = std::reduce(res.begin(), res.end());
    const auto avg = static_cast<double>(sum) / static_cast<double>(count);

    std::cout << "Iterations: " << count << std::endl
              << "Size:       " << size << " Byte (" << size + sizeof(ipc::DataHeader) << " Byte)" << std::endl
              << "Minimum:    " << *min / 1000.0 << "us" << std::endl
              << "Maximum:    " << *max / 1000.0 << "us" << std::endl
              << "Average:    " << avg / 1000.0 << "us" << std::endl;

#if 0
    std::cout << "Data: ";
    for (auto &i: bench.get_results())
        std::cout << i << ' ';
    std::cout << std::endl;
#endif

    return EXIT_SUCCESS;
}

int run_real_world(ipc::ICommunicationHandler &handler, const std::string &path, bool readonly) {
    std::vector<ipc::benchmark::RealWorldBenchmark::DataPoint> data{};

    std::ifstream file(path);
    std::string str;
    while (std::getline(file, str)) {
        std::stringstream ss(str);
        std::string item;
        std::vector<std::string> elems;
        while (std::getline(ss, item, ' '))
            elems.push_back(item);

        std::int64_t time = std::stoll(elems[2].erase(0, 1));
        std::uint64_t address = std::stoull(elems[3], nullptr, 16);
        std::uint32_t length = std::stoul(elems[4].erase(elems[4].size() - 1));

        std::string name = elems[5];
        for (auto p = elems.begin() + 6; p < elems.end(); ++p) {
            name += ' ' + *p;
        }

        name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());

        data.push_back({time, ipc::JavaSymbol(address, length, name)});
    }
    file.close();

    ipc::benchmark::RealWorldBenchmark bench(data, 5, readonly);
    if (!bench.setup(handler))
        return EXIT_FAILURE;

    std::cout << "Running Latency benchmark..." << std::endl;
    const auto success = bench.run(handler);
    std::cout << "Benchmark completed!" << std::endl;

    bench.cleanup(handler);

    if (!success)
        return EXIT_FAILURE;

    const auto count = bench.get_iterations();
    const auto threshold = bench.get_threshold();
    const auto misses = bench.get_misses();

    std::cout << "Iterations: " << count << std::endl
              << "Threshold:  " << threshold << "us" << std::endl
              << "Misses:     " << misses << std::endl;

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Missing arguments" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string type(argv[1]);
    const std::string path(argv[2]);
    const auto readonly = strcmp(argv[3], "reader") == 0;

    auto handler = create_handler(type, path, readonly);
    if (!handler) {
        std::cout << "Invalid parameter" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Loading handler... (" << type << ')' << std::endl;

#if 1
    auto temp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "Start: " << ctime(&temp);

    // return run_latency(*handler, readonly);
    // return run_throughput(*handler, readonly);
    // return run_execution_time(*handler, readonly);
    auto res = run_real_world(*handler, "../testdata/fifo_trace.txt", readonly);

    temp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "End: " << ctime(&temp);

    return res;
#else
    const auto res = handler->open();
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

    std::cout << "Shutting down..." << std::endl;
    stop = true;
    t.join();

    handler->close();
    std::cout << "Handler closed" << std::endl;

    return EXIT_SUCCESS;
#endif
}
