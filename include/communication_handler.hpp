#pragma once

#include <memory>
#include <tuple>
#include <variant>

#include "communication_error.hpp"
#include "data_object.hpp"
#include "data_header.hpp"
#include "java_symbol.hpp"

namespace ipc {

/// Variant for all data types
using DataObject = std::variant<JavaSymbol>;

/**
 * Interface for all inter-process communication handlers.
 */
class ICommunicationHandler {
public:
    /// Time to wait for each poll in milliseconds.
    static constexpr short WAIT_TIME = 5000;

    virtual ~ICommunicationHandler() = default;

    /**
     * Open handler.
     *
     * @return True, if handler was successfully opened.
     */
    virtual bool open() = 0;

    /**
     * Close the handler.
     *
     * @return True, if handler was successfully closed.
     */
    virtual bool close() = 0;

    /**
     * Poll new data from the handler.
     *
     * @return True, if poll was successful.
     * @remark Method will block until an event occurred.
     */
    virtual bool await_data() const = 0;

    /**
     * Check if new data is available.
     *
     * @return True, if data is available.
     */
    virtual bool has_data() const = 0;

    /**
     * Write a data object into the inter-process communication handler.
     *
     * @param obj Object to write into the handler.
     */
    virtual bool write(const IDataObject &obj) = 0;

    /**
     * Read a data objects from the inter-process communication handler.
     *
     * @return Objects received from the handler or an error.
     */
    virtual std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() = 0;
};

}