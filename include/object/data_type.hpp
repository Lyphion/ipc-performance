#pragma once

#include <cstdint>

namespace ipc {

/**
 * Enumeration of all data types.
 */
enum class DataType : std::uint16_t {
    /// Invalid type
    INVALID = 0,

    /// Type for Java Symbols (ipc::Ping)
    PING = 1,

    /// Type for Java Symbols (ipc::JavaSymbol)
    JAVA_SYMBOL_LOOKUP = 2,

    /// Type for Binary Data (ipc::BinaryData)
    BINARY_DATA = 3,
};

}
