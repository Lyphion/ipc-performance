#include "benchmark/throughput.hpp"

#include <iostream>
#include <thread>
#include <vector>

#include "object/binary_data.hpp"
#include "utility.hpp"

namespace ipc::benchmark {

ThroughputBenchmark::ThroughputBenchmark(unsigned int iterations, unsigned int size, bool server)
        : iterations_(iterations), size_(size), server_(server) {}

bool ThroughputBenchmark::run(ICommunicationHandler &handler) {
    return server_ ? run_server(handler) : run_client(handler);
}

bool ThroughputBenchmark::setup(ICommunicationHandler &handler) {
    return handler.open();
}

bool ThroughputBenchmark::run_server(ICommunicationHandler &handler) {
    constexpr auto max_retries = 10 * 1000 / ICommunicationHandler::WAIT_TIME;
    auto more_data = false;

    while (received_ < iterations_) {
        // Wait for new messages
        auto retry = 0;
        while (!more_data && !handler.await_data()) {
            retry++;

            if (received_ > 0 && retry > max_retries)
                return true;
        }

        // Read messages
        const auto result = handler.read();
        more_data = !std::holds_alternative<ipc::CommunicationError>(result);

        // Handle result
        const auto success = std::visit(overloaded{
                [this](const ipc::CommunicationError &error) {
                    // 'No data available' is not a real error, so ignore it
                    if (error == ipc::CommunicationError::NO_DATA_AVAILABLE)
                        return true;

                    std::cout << "Error reading data on iteration " << received_
                              << " (Error: " << static_cast<int>(error) << ')' << std::endl;
                    return false;
                },
                [this](const auto &success) {
                    const DataHeader header = std::get<DataHeader>(success);

                    if (received_ == 1)
                        start_time_ = header.get_timestamp();
                    end_time_ = ipc::get_timestamp();
                    received_++;

                    // std::cout << "Data " << received_ << " " << header << std::endl;

                    return true;
                }
        }, result);

        if (!success)
            return false;
    }

    return true;
}

bool ThroughputBenchmark::run_client(ICommunicationHandler &handler) const {
    // Package size must account size of vector
    const auto amount = size_ - sizeof(std::uint32_t);

    // Construct dummy data
    std::vector<std::byte> b{};
    for (unsigned int i = 0; i < amount; ++i) {
        b.push_back(static_cast<std::byte>(rand() % 256));
    }
    const BinaryData data(b);

    for (unsigned int i = 1; i <= iterations_; ++i) {
        const auto result = handler.write(data);

        if (!result) {
            std::cout << "Error writing data on iteration " << i << std::endl;
            return false;
        }

        // std::cout << "Data " << i << std::endl;
    }

    return true;
}

void ThroughputBenchmark::cleanup(ICommunicationHandler &handler) {
    handler.close();
}

}
