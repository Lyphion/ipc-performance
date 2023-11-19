#pragma once

#include <optional>
#include <ostream>

#include "data_object.hpp"

namespace ipc {

/**
 * Empty object for pings.
 */
class Ping : public IDataObject {
public:
    /**
     * Create a new ping object.
     */
    Ping() = default;

    ~Ping() override = default;

    int serialize(std::byte *buffer, unsigned int size) const override;

    inline DataType get_type() const override { return DataType::PING; };

    /**
     * Deserialize the object from a buffer.
     *
     * @param buffer Buffer to deserialize the object from.
     * @param size   Size of the buffer.
     *
     * @return Deserialized object from buffer.
     */
    static std::optional<Ping> deserialize(const std::byte *buffer, unsigned int size);
};

std::ostream &operator<<(std::ostream &outs, const Ping &ping);

}
