#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "data_object.hpp"

namespace ipc {

/**
 * Object with binary data of fixed size.
 */
class BinaryData : public IDataObject {
public:
    /**
     * Create a new binary data object of fixed size.
     *
     * @param data Binary data.
     */
    explicit BinaryData(std::vector<std::byte> data);

    ~BinaryData() override = default;

    int serialize(std::byte *buffer, unsigned int size) const override;

    DataType get_type() const override { return DataType::BINARY_DATA; };

    /**
     * Data of this object.
     */
    const std::vector<std::byte> &get_data() const { return data_; }

    /**
     * Deserialize the object from a buffer.
     *
     * @param buffer Buffer to deserialize the object from.
     * @param size   Size of the buffer.
     *
     * @return Deserialized object from buffer.
     */
    static std::optional<BinaryData> deserialize(const std::byte *buffer, unsigned int size);

private:
    const std::vector<std::byte> data_;

};

std::ostream &operator<<(std::ostream &outs, const BinaryData &data);

}
