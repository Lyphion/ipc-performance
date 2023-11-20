#include "object/data_header.hpp"

#include <cstring>
#include <iomanip>

namespace ipc {

DataHeader::DataHeader(std::uint32_t id, DataType type, std::uint16_t body_size, std::int64_t timestamp)
        : id_(id), type_(type), body_size_(body_size), timestamp_(timestamp) {}

int DataHeader::serialize(std::byte *buffer, const unsigned int size) const {
    constexpr auto header_size = sizeof(DataHeader);
    static_assert(header_size == 16, "Size of header should match");

    // Not enough space in buffer
    if (size < header_size)
        return -1;

    const auto header_data = reinterpret_cast<const char *>(this);
    std::memcpy(buffer, header_data, header_size);
    return header_size;
}

std::optional<DataHeader> DataHeader::deserialize(const std::byte *buffer, unsigned int size) {
    constexpr auto header_size = sizeof(DataHeader);
    static_assert(header_size == 16, "Size of header should match");

    // Not enough space in buffer
    if (size < header_size)
        return std::nullopt;

    DataHeader header(0, DataType::INVALID, 0, 0);
    const auto header_data = reinterpret_cast<char *>(&header);
    std::memcpy(header_data, buffer, header_size);

    return header;
}

std::ostream &operator<<(std::ostream &outs, const DataHeader &header) {
    std::ios_base::fmtflags f(outs.flags());

    outs << std::setfill('0')
         << '(' << header.get_id() << ", " << static_cast<std::uint16_t>(header.get_type())
         << ", 0x" << std::hex << std::setw(sizeof(header.get_body_size()) * 2) << header.get_body_size()
         << ", " << std::dec << header.get_timestamp() << ')';
    outs.flags(f);

    return outs;
}

}