#pragma once

#include "handler/communication_handler.hpp"

namespace ipc::benchmark {

/**
 * Simple interface for benchmarks.
 */
class IBenchmark {
public:
    /**
     * Setup tests.
     *
     * @param handler Communication handler to run the tests on.
     *
     * @return True, if setup was successful.
     */
    virtual bool setup(ICommunicationHandler &handler) = 0;

    /**
     * Run the current benchmark on the handler.
     *
     * @param handler Communication handler to run the tests on.
     *
     * @return True, if benchmark was successful.
     */
    virtual bool run(ICommunicationHandler &handler) = 0;

    /**
     * Cleanup tests.
     *
     * @param handler Communication handler to run the tests on.
     */
    virtual void cleanup(ICommunicationHandler &handler) = 0;
};

}
