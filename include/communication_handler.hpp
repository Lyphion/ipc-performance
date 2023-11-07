#pragma once

#include <memory>
#include <tuple>
#include <variant>
#include <vector>

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
     * Read a data objects from the inter-process communication handler.
     *
     * @return Objects received from the handler.
     */
    virtual std::vector<std::tuple<DataHeader, DataObject>> read() = 0;
};

}