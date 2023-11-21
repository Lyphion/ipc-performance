#pragma once

#include <cstdint>
#include <vector>

#include "benchmark.hpp"

namespace ipc::benchmark {

/**
 * Real world benchmark of the communication handlers with fixed messages.
 */
class RealWorldBenchmark : public IBenchmark {
public:
    /// Datapoint for each message.
    struct DataPoint {
        /// Time in nanoseconds since epoch.
        std::int64_t time;
        /// Data to send/receive.
        JavaSymbol symbol;
    };

public:
    /**
     * Create new real world benchmark with fixed messages.
     *
     * @param data      Data to send or receive.
     * @param threshold Maximum deadline threshold in microseconds per message.
     * @param server    If the server side should be executed.
     */
    RealWorldBenchmark(std::vector<DataPoint> data, unsigned int threshold, bool server);

    bool setup(ICommunicationHandler &handler) override;

    bool run(ICommunicationHandler &handler) override;

    void cleanup(ICommunicationHandler &handler) override;

    /**
     * Maximum deadline threshold in microseconds per message.
     */
    unsigned int get_threshold() const { return threshold_; }

    /**
     * Amount if data points.
     */
    unsigned int get_iterations() const { return data_.size(); }

    /**
     * Return the estimated runtime in seconds.
     */
    double get_estimated_time() const { return static_cast<double>(data_.back().time - data_.front().time) / 1000.0 / 1000.0 / 1000.0; }

    /**
     * Return the amount of deadline misses or lost packages of the benchmark.
     *
     * @remarks Only valid if benchmark completed successfully.
     */
    unsigned int get_misses() const { return misses_; }

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
    const std::vector<DataPoint> data_;
    const unsigned int threshold_;
    const bool server_;

    unsigned int misses_ = 0;
};

}
