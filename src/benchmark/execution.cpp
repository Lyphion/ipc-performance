#include "benchmark/execution.hpp"

#include <iostream>
#include <thread>

#include "utility.hpp"

namespace ipc::benchmark {

ExecutionTimeBenchmark::ExecutionTimeBenchmark(unsigned int iterations, unsigned int size, unsigned int delay, bool server)
        : iterations_(iterations), size_(size), delay_(delay), server_(server) {}

bool ExecutionTimeBenchmark::setup(ICommunicationHandler &handler) {
    execution_times_.reserve(iterations_);
    return handler.open();
}

bool ExecutionTimeBenchmark::run(ICommunicationHandler &handler) {
    return server_ ? run_server(handler) : run_client(handler);
}

bool ExecutionTimeBenchmark::run_server(ICommunicationHandler &handler) {
    auto more_data = false;
    unsigned int i = 1;

    while (i <= iterations_) {
        // Wait for new messages
        while (!more_data && !handler.await_data());

        // Read messages
        const auto before = ipc::get_timestamp();
        const auto result = handler.read();
        const auto after = ipc::get_timestamp();
        more_data = !std::holds_alternative<ipc::CommunicationError>(result);

        if (more_data) {
            const auto delta = after - before;
            execution_times_.push_back(delta);
        }

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
                [&i](const auto &) {
                    i++;
                    return true;
                }
        }, result);

        if (!success)
            return false;
    }

    return true;
}

bool ExecutionTimeBenchmark::run_client(ICommunicationHandler &handler) {
    // Package size must account size of vector
    const auto amount = size_ - sizeof(std::uint32_t);

    // Construct dummy data
    std::vector<std::byte> b{};
    for (unsigned int i = 0; i < amount; ++i) {
        b.push_back(static_cast<std::byte>(rand() % 256));
    }
    const BinaryData data(b);

    const auto delay = std::chrono::milliseconds(delay_);

    for (unsigned int i = 1; i <= iterations_; ++i) {
        const auto before = ipc::get_timestamp();
        const auto result = handler.write(data);
        const auto after = ipc::get_timestamp();

        const auto delta = after - before;
        execution_times_.push_back(delta);

        if (!result) {
            std::cout << "Error writing data on iteration " << i << std::endl;
            return false;
        }
        // Sleep a bit before sending next message
        std::this_thread::sleep_for(delay);
    }

    return true;
}

void ExecutionTimeBenchmark::cleanup(ICommunicationHandler &handler) {
    handler.close();
}

}