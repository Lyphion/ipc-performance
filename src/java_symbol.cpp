#include "java_symbol.hpp"

#include <cstring>
#include <iomanip>

namespace ipc {

JavaSymbol::JavaSymbol(std::uint64_t address, std::uint32_t length, std::string symbol)
        : address_(address), length_(length), symbol_(std::move(symbol)) {}

int JavaSymbol::serialize(std::byte *buffer, unsigned int size) const {
    constexpr auto header_size = sizeof(JavaSymbol::address_)
                                 + sizeof(JavaSymbol::length_) + sizeof(std::uint32_t);
    static_assert(header_size == 16, "Size should match");

    const auto obj_size = header_size + this->symbol_.length();

    // Not enough space in buffer
    if (obj_size > size)
        return -1;

    auto offset = 0;
    std::memcpy(&buffer[offset], &this->address_, sizeof(JavaSymbol::address_));
    offset += sizeof(this->address_);

    std::memcpy(&buffer[offset], &this->length_, sizeof(JavaSymbol::length_));
    offset += sizeof(this->length_);

    std::uint32_t length = this->symbol_.length();
    std::memcpy(&buffer[offset], &length, sizeof(length));
    offset += sizeof(length);

    std::memcpy(&buffer[offset], this->symbol_.data(), length);

    return static_cast<int>(obj_size);
}

std::optional<JavaSymbol> JavaSymbol::deserialize(const std::byte *buffer, unsigned int size) {
    constexpr auto header_size = sizeof(JavaSymbol::address_)
                                 + sizeof(JavaSymbol::length_) + sizeof(std::uint32_t);
    static_assert(header_size == 16, "Size should match");

    std::uint64_t address;
    std::uint32_t length, symbol_length;

    // Not enough space in buffer
    if (size < header_size)
        return std::nullopt;

    auto offset = 0;
    std::memcpy(&address, &buffer[offset], sizeof(address));
    offset += sizeof(address);

    std::memcpy(&length, &buffer[offset], sizeof(length));
    offset += sizeof(length);

    std::memcpy(&symbol_length, &buffer[offset], sizeof(symbol_length));
    offset += sizeof(symbol_length);

    // Not enough space in buffer
    if (size < header_size + symbol_length)
        return std::nullopt;

    // Empty symbol name
    if (symbol_length == 0)
        return JavaSymbol(address, length, "");

    std::string symbol;
    symbol.resize(symbol_length);
    std::memcpy(symbol.data(), &buffer[offset], symbol_length);

    return JavaSymbol(address, length, symbol);
}

std::ostream &operator<<(std::ostream &outs, const JavaSymbol &symbol) {
    std::ios_base::fmtflags f(outs.flags());

    outs << std::setfill('0') << std::hex
         << "(0x" << std::setw(sizeof(symbol.get_address()) * 2) << symbol.get_address()
         << ", 0x" << std::setw(sizeof(symbol.get_length()) * 2) << symbol.get_length()
         << ", \"" << symbol.get_symbol() << "\")";
    outs.flags(f);

    return outs;
}

}
