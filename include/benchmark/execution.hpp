#pragma once

#include <vector>

#include "benchmark.hpp"

namespace ipc::benchmark {

/**
 * Execution time benchmark of the communication handlers with different package sizes.
 */
class ExecutionTimeBenchmark : public IBenchmark {
public:
    /**
     * Create new execution time benchmark with fixed amount of iterations and package size.
     *
     * @param iterations Number of iterations.
     * @param size       Size of the package body.
     * @param delay      Delay in milliseconds between iterations.
     * @param server     If the server side should be executed.
     */
    ExecutionTimeBenchmark(unsigned int iterations, unsigned int size, unsigned int delay, bool server);

    bool setup(ICommunicationHandler &handler) override;

    bool run(ICommunicationHandler &handler) override;

    void cleanup(ICommunicationHandler &handler) override;

    /**
     * Return the results of the benchmark.
     *
     * @remarks Only valid if benchmark completed successfully.
     */
    const std::vector<unsigned int> &get_results() { return execution_times_; }

    /**
     * Amount if iterations.
     */
    unsigned int get_iterations() const { return iterations_; }

    /**
     * Size of the body of one package.
     */
    unsigned int get_size() const { return size_; }

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
    bool run_client(ICommunicationHandler &handler);

private:
    const unsigned int iterations_;
    const unsigned int size_;
    const unsigned int delay_;
    const bool server_;

    std::vector<unsigned int> execution_times_{};
};

}
