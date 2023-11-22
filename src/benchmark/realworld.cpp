#include "benchmark/realworld.hpp"

#include <iostream>
#include <thread>
#include <utility>

#include "utility.hpp"

namespace ipc::benchmark {

RealWorldBenchmark::RealWorldBenchmark(std::vector<DataPoint> data, unsigned int threshold, bool server)
        : data_(std::move(data)), threshold_(threshold), server_(server) {}

bool RealWorldBenchmark::setup(ICommunicationHandler &handler) {
    return handler.open();
}

bool RealWorldBenchmark::run(ICommunicationHandler &handler) {
    return server_ ? run_server(handler) : run_client(handler);
}

bool RealWorldBenchmark::run_server(ICommunicationHandler &handler) {
    auto more_data = false;
    unsigned int i = 0;

    std::cout << "Estimated time: " << get_estimated_time() << "s" << std::endl;

    while (i < data_.size()) {
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

                    std::cout << "Error reading data on iteration " << i + 1
                              << " (Error: " << static_cast<int>(error) << ')' << std::endl;
                    return false;
                },
                [this, &i](const auto &success) {
                    const DataHeader header = std::get<DataHeader>(success);

                    misses_ += header.get_id() - i - 1;
                    i = header.get_id();

                    // std::cout << "Data " << i << " " << header << std::endl;
                    return true;
                }
        }, result);

        if (!success)
            return false;
    }

    return true;
}

bool RealWorldBenchmark::run_client(ICommunicationHandler &handler) {
    std::cout << "Estimated time: " << get_estimated_time() << "s" << std::endl;

    const std::int64_t start_delay = 1 * 1000 * 1000 * 1000;
    const std::int64_t delta = ipc::get_timestamp() - data_[0].time + start_delay;
    const auto threshold = threshold_ * 1000;

    for (unsigned int i = 0; i < data_.size(); ++i) {
        const auto time = data_[i].time + delta;
        const auto now = ipc::get_timestamp() - threshold;

        if (time < now) {
            misses_++;
        } else {
            const auto target = std::chrono::nanoseconds(time);
            std::chrono::time_point<std::chrono::high_resolution_clock> dt(target);
            std::this_thread::sleep_until(dt);
        }

        const auto result = handler.write(data_[i].symbol);

        if (!result) {
            std::cout << "Error writing data on iteration " << i + 1 << std::endl;
            return false;
        }

        // std::cout << "Data " << i << std::endl;
    }

    return true;
}

void RealWorldBenchmark::cleanup(ICommunicationHandler &handler) {
    handler.close();
}

}
