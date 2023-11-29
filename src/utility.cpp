#include "utility.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>

extern "C" {
#include <sys/poll.h>
}

namespace ipc {

void print_array(const std::byte *data, unsigned int size) {
    std::ios_base::fmtflags f(std::cout.flags());

    std::cout << std::setfill('0') << std::hex;
    for (unsigned int i = 0; i < size; i++) {
        std::cout << std::setw(sizeof(char) * 2) << static_cast<int>(data[i]);
    }

    std::cout << std::endl;
    std::cout.flags(f);
}

std::int64_t get_timestamp() {
    const auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(time)
            .time_since_epoch().count();
}

int poll(int fd, int timeout) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;

    // Poll events and block for 'timeout'
    return ::poll(&pfd, 1, timeout);
}

std::variant<DataObject, CommunicationError> deserialize_data_object(DataType type, const std::byte *buffer, unsigned int size) {
    // Handle each type differently
    switch (type) {
        case DataType::INVALID:
            return CommunicationError::INVALID_DATA;

        case DataType::PING: {
            // Deserialize Ping
            const auto data = Ping::deserialize(buffer, size);
            if (!data)
                return CommunicationError::INVALID_DATA;

            return *data;
        }

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            const auto data = JavaSymbol::deserialize(buffer, size);
            if (!data)
                return CommunicationError::INVALID_DATA;

            return *data;
        }

        case DataType::BINARY_DATA: {
            // Deserialize Binary data
            const auto data = BinaryData::deserialize(buffer, size);
            if (!data)
                return CommunicationError::INVALID_DATA;

            return *data;
        }
    }

    // Unknown or invalid type
    return CommunicationError::UNKNOWN_DATA;
}

}