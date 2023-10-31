#pragma once

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
     * @remark It will block until the object was written.
     */
    virtual void write(const IDataObject &obj) = 0;

    /**
     * Read a data object from the inter-process communication handler.
     *
     * @param header Received header of the object.
     *
     * @return Object received from the handler.
     * @remark It will block until a object was read.
     */
    virtual std::tuple<DataHeader, IDataObject> read() = 0;
};

}