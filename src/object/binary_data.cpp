#include "object/binary_data.hpp"

#include <cstring>
#include <iomanip>
#include <utility>

namespace ipc {

BinaryData::BinaryData(std::vector<std::byte> data)
        : data_(std::move(data)) {}

int BinaryData::serialize(std::byte *buffer, unsigned int size) const {
    const auto total_size = sizeof(std::uint32_t) + data_.size();

    // Not enough space in buffer
    if (total_size > size)
        return -1;

    auto offset = 0;
    const std::uint32_t length = data_.size();
    std::memcpy(&buffer[offset], &length, sizeof(length));
    offset += sizeof(length);

    std::memcpy(&buffer[offset], this->data_.data(), length);

    return static_cast<int>(total_size);
}

std::optional<BinaryData> BinaryData::deserialize(const std::byte *buffer, unsigned int size) {
    constexpr auto header_size = sizeof(std::uint32_t);

    // Not enough space in buffer
    if (size < header_size)
        return std::nullopt;

    std::uint32_t length;

    auto offset = 0;
    std::memcpy(&length, &buffer[offset], sizeof(length));
    offset += sizeof(length);

    // Not enough space in buffer
    if (size < header_size + length)
        return std::nullopt;

    // Empty data
    if (length == 0)
        return BinaryData(std::vector<std::byte>{});

    std::vector<std::byte> data_{};
    data_.reserve(length);
    std::memcpy(data_.data(), &buffer[offset], length);

    return BinaryData(data_);
}

std::ostream &operator<<(std::ostream &outs, const BinaryData &data) {
    std::ios_base::fmtflags f(outs.flags());
    outs << std::setfill('0') << std::hex << '(';

    for (const auto i : data.get_data()) {
        outs << std::setw(sizeof(char) * 2) << static_cast<int>(i);
    }

    outs << ')';
    outs.flags(f);

    return outs;
}

}
