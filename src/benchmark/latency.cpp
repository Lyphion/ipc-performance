#include "benchmark/latency.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "utility.hpp"

namespace ipc::benchmark {

LatencyBenchmark::LatencyBenchmark(unsigned int iterations, unsigned int delay, bool server)
        : iterations_(iterations), delay_(delay), server_(server) {}

bool LatencyBenchmark::run(ICommunicationHandler &handler) {
    return server_ ? run_server(handler) : run_client(handler);
}

bool LatencyBenchmark::setup(ICommunicationHandler &handler) {
    latencies_.reserve(iterations_);
    return handler.open();
}

bool LatencyBenchmark::run_server(ICommunicationHandler &handler) {
    auto more_data = false;
    unsigned int i = 1;

    while (i <= iterations_) {
        // Wait for new messages
        while (!more_data && !handler.await_data());

        // Read messages
        const auto result = handler.read();
        more_data = !std::holds_alternative<ipc::CommunicationError>(result);

        // Handle result
        const auto success = std::visit(overloaded{
                [&i](const ipc::CommunicationError &error) {
                    // 'No data available' is not a real error, so ignore it
                    if (error == ipc::CommunicationError::NO_DATA_AVAILABLE)
                        return true;

                    std::cout << "Error reading data on iteration " << i
                              << " (Error: " << static_cast<int>(error) << ')' << std::endl;
                    return false;
                },
                [this, &i](const auto &success) {
                    const DataHeader header = std::get<DataHeader>(success);
                    const auto ts = ipc::get_timestamp();

                    // Compute latency from creation to now
                    const auto delta = ts - header.get_timestamp();
                    latencies_.push_back(delta);
                    i++;

                    return true;
                }
        }, result);

        if (!success)
            return false;
    }

    return true;
}

bool LatencyBenchmark::run_client(ICommunicationHandler &handler) const {
    const Ping data{};
    const auto delay = std::chrono::milliseconds(delay_);

    for (unsigned int i = 1; i <= iterations_; ++i) {
        const auto result = handler.write(data);

        if (!result) {
            std::cout << "Error writing data on iteration " << i << std::endl;
            return false;
        }
        // Sleep a bit before sending next message
        std::this_thread::sleep_for(delay);
    }

    return true;
}

void LatencyBenchmark::cleanup(ICommunicationHandler &handler) {
    handler.close();
}

}
