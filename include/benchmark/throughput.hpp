#pragma once

#include <cstdint>

#include "benchmark.hpp"

namespace ipc::benchmark {

/**
 * Throughput benchmark of the communication handlers with different package sizes.
 */
class ThroughputBenchmark : public IBenchmark {
public:
    /**
     * Create new throughput benchmark with fixed amount of iterations and package size.
     *
     * @param iterations Number of iterations.
     * @param size       Size of the package body.
     * @param server     If the server side should be executed.
     */
    ThroughputBenchmark(unsigned int iterations, unsigned int size, bool server);

    bool setup(ICommunicationHandler &handler) override;

    bool run(ICommunicationHandler &handler) override;

    void cleanup(ICommunicationHandler &handler) override;

    /**
     * Return the total amount if time in milliseconds for of the benchmark.
     *
     * @remarks Only valid if benchmark completed successfully.
     */
    double get_total_time() const { return static_cast<double>(end_time_ - start_time_) / 1000.0 / 1000.0; }

    /**
     * Return the throughput in KiB/s for of the benchmark.
     *
     * @remarks Only valid if benchmark completed successfully.
     */
    double get_throughput() const { return (received_ * size_ / 1024.0) / (get_total_time() / 1000.0); }

    /**
     * Amount if iterations.
     */
    unsigned int get_iterations() const { return iterations_; }

    /**
     * Size of the body of one package.
     */
    unsigned int get_size() const { return size_; }

    /**
     * Amount if messages received. This value should match iterations.
     */
    unsigned int get_received() const { return received_; }

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
    const unsigned int size_;
    const bool server_;

    std::int64_t start_time_ = 0;
    std::int64_t end_time_ = 0;
    unsigned int received_ = 0;
};

}
