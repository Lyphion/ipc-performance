#pragma once

#include "data_type.hpp"

namespace ipc {

/**
 * Interface for all data objects.
 */
class IDataObject {
public:
    /**
     * Destructor for the data objects.
     */
    virtual ~IDataObject() = default;

    /**
     * Serialize the object into a buffer.
     *
     * @param buffer Buffer to serialize the object into.
     * @param size   Size of the buffer.
     *
     * @return Total number of bytes written into the buffer.
     */
    virtual unsigned int serialize(char *buffer, unsigned int size) const = 0;

    /**
     * Type of the object.
     */
    virtual DataType get_type() const = 0;
};

}
