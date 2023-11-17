#pragma once

namespace ipc {

/**
 * Enumeration representing errors of the connection handler.
 */
enum class CommunicationError {
    /// Connection has been closed
    CONNECTION_CLOSED = 0,

    /// No data available to read
    NO_DATA_AVAILABLE = 1,

    /// Error while reading
    READ_ERROR = 2,

    /// Invalid format of header
    INVALID_HEADER = 3,

    /// Invalid data received
    INVALID_DATA = 4,

    /// Unknown type or data received
    UNKNOWN_DATA = 5
};

}