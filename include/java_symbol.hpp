#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include "data_object.hpp"

namespace ipc {

/**
 * Object with Java Symbol and address area.
 */
class JavaSymbol : public IDataObject {
public:
    /**
     * Create a new data java symbol object containing address area and name.
     *
     * @param address Address of the symbol.
     * @param length  Length of the symbol address area.
     * @param symbol  Name of the symbol.
     */
    JavaSymbol(std::uint64_t address, std::uint32_t length, std::string symbol)
            : address_(address), length_(length), symbol_(std::move(symbol)) {}

    /**
     * Default destructor.
     */
    ~JavaSymbol() override = default;

    int serialize(char *buffer, unsigned int size) const override;

    inline DataType get_type() const override { return DataType::JAVA_SYMBOL_LOOKUP; };

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
    static std::optional<JavaSymbol> deserialize(const char *buffer, unsigned int size);

private:
    std::uint64_t address_;
    std::uint32_t length_;
    std::string symbol_;
};

std::ostream &operator<<(std::ostream &outs, const JavaSymbol &symbol);

}
