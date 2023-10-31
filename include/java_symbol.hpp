#pragma once

#include <string>
#include <utility>

#include "data_object.hpp"

namespace ipc {

class JavaSymbol : public IDataObject {
public:
    JavaSymbol(std::uint64_t address, std::uint32_t length, std::string symbol)
            : address_(address), length_(length), symbol_(std::move(symbol)) {}

    unsigned int serialize(char *buffer, unsigned int size) const override;

    DataType get_type() const override { return DataType::JAVA_SYMBOL_LOOKUP; };

    /**
     * Address of the symbol.
     */
    std::uint64_t get_address() const { return address_; }

    /**
     * Length of the address area of the symbol.
     */
    std::uint32_t get_length() const { return length_; }

    /**
     * Name of the symbol.
     */
    const std::string &get_symbol() const { return symbol_; }

    /**
     * Deserialize the object from a buffer.
     *
     * @param buffer Buffer to deserialize the object from.
     * @param size   Size of the buffer.
     *
     * @return Deserialize object from buffer.
     */
    static JavaSymbol deserialize(const char *buffer, unsigned int size);

private:
    std::uint64_t address_;
    std::uint32_t length_;
    std::string symbol_;
};

}
