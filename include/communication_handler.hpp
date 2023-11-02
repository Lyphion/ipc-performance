#pragma once

#include <memory>
#include <tuple>

#include "data_object.hpp"
#include "data_header.hpp"

namespace ipc {

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
    virtual std::tuple<DataHeader, std::unique_ptr<IDataObject>> read() = 0;
};

}