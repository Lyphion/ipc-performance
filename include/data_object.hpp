#pragma once

#include "data_type.hpp"

namespace ipc {

class IDataObject {
public:
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
     * Type of the Object.
     */
    virtual DataType get_type() const = 0;
};

}
