#pragma once

#include <vector>

#include "benchmark/benchmark.hpp"
#include "handler/communication_handler.hpp"

namespace ipc::benchmark {

/**
 * Latency benchmark of the communication handlers with empty message.
 */
class LatencyBenchmark : public IBenchmark {
public:
    /**
     * Create new latency benchmark with fixed amount of iterations.
     *
     * @param iterations Number of iterations.
     * @param delay      Delay in milliseconds between iterations.
     * @param server     If the server side should be executed.
     */
    LatencyBenchmark(unsigned int iterations, unsigned int delay, bool server);

    bool setup(ICommunicationHandler &handler) override;

    bool run(ICommunicationHandler &handler) override;

    void cleanup(ICommunicationHandler &handler) override;

    /**
     * Return the results of the benchmark.
     *
     * @remarks Only valid if benchmark completed successfully.
     */
    const std::vector<unsigned int> &get_results() { return latencies_; }

    /**
     * Amount if iterations.
     */
    unsigned int get_iterations() const { return iterations_; }

    /**
     * Delay between write operations.
     */
    unsigned int get_delay() const { return delay_; }

private:
    /**
     * Run the server part of the benchmark.
     *
     * @param handler Communication handler to run the tests on.
     *
     * @return True, if benchmark was successful.
     */
    bool run_server(ICommunicationHandler &handler);

    /**
     * Run the client part of the benchmark.
     *
     * @param handler Communication handler to run the tests on.
     *
     * @return True, if benchmark was successful.
     */
    bool run_client(ICommunicationHandler &handler) const;

private:
    const unsigned int iterations_;
    const unsigned int delay_;
    const bool server_;

    std::vector<unsigned int> latencies_{};
};

}

