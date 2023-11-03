#pragma once

#include <memory>
#include <optional>
#include <tuple>
#include <variant>

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
    /**
     * Write a data object into the inter-process communication handler.
     *
     * @param obj Object to write into the handler.
     */
    virtual bool write(const IDataObject &obj) = 0;

    /**
     * Read a data object from the inter-process communication handler.
     *
     * @param header Received header of the object.
     *
     * @return Object received from the handler.
     */
    virtual std::optional<std::tuple<DataHeader, DataObject>> read() = 0;
};

}