#pragma once

#include <cstdint>

namespace ipc {

enum DataType : std::uint16_t {
    INVALID = 0,
    JAVA_SYMBOL_LOOKUP = 1
};

}
