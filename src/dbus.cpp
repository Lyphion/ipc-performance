#include "../include/dbus.hpp"

#include "utility.hpp"

namespace ipc {

DBus::DBus(std::string name, bool server)
        : name_(std::move(name)), server_(server) {}

DBus::~DBus() {
    if (con_ != nullptr) {
        DBus::close();
    }
}

bool DBus::open() {
    // Check if dbus is already open
    if (con_ != nullptr)
        return true;

    // Create server or client
    if (server_) {
        if (!create_server()) {
            close();
            return false;
        }
    } else if (!create_client()) {
        close();
        return false;
    }

    return true;
}

bool DBus::create_server() {
    DBusError err;
    dbus_error_init(&err);

    // connect to the dbus and check for errors
    con_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (con_ == nullptr) {
        return false;
    }

    // request our name on the bus and check for errors
    auto ret = dbus_bus_request_name(con_, name_.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return false;
    }

    return true;
}

bool DBus::create_client() {
    DBusError err;
    dbus_error_init(&err);

    // connect to the dbus and check for errors
    con_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (con_ == nullptr) {
        return false;
    }

    return true;
}

bool DBus::close() {
    // Check if dbus is already closed
    if (con_ == nullptr)
        return false;

    DBusError err;
    dbus_error_init(&err);

    if (server_)
        dbus_bus_release_name(con_, name_.c_str(), &err);

    con_ = nullptr;

    return true;
}

bool DBus::await_data() const {
    // Check if dbus is open
    if (con_ == nullptr)
        return false;

    // Poll events and block until one is available
    auto res = dbus_connection_read_write(con_, -1);
    return res != 0;
}

bool DBus::has_data() const {
    // Check if dbus is open
    if (con_ == nullptr)
        return false;

    // Poll events and block for 1ms
    auto res = dbus_connection_read_write(con_, 1);
    return res != 0;
}

bool DBus::write(const IDataObject &obj) {
    // Check if dbus is open
    if (con_ == nullptr)
        return false;

    DBusError err;
    dbus_error_init(&err);

    // Build message object path
    auto path = PATH_PREFIX + std::to_string(static_cast<int>(obj.get_type()));

    // Prepare message
    auto msg = dbus_message_new_method_call(
            name_.c_str(),          // target for the method call
            path.c_str(),           // object to call on
            INTERFACE_NAME.c_str(), // interface to call on
            METHOD_NAME.c_str());   // method to call

    if (msg == nullptr) {
        dbus_message_unref(msg);
        return false;
    }

    // Serialize body
    auto size = obj.serialize(buffer_.data(), BUFFER_SIZE);
    if (size == -1)
        return false;

    last_id_++;
    auto timestamp = get_timestamp();

    // Prepare arguments
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);

    // Append id
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &last_id_)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    // Append timestamp
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT64, &timestamp)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    // Append object data
    DBusMessageIter arr;
    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &arr)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    auto ptr = buffer_.data();
    if (!dbus_message_iter_append_fixed_array(&arr, DBUS_TYPE_BYTE, &ptr, size)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    if (!dbus_message_iter_close_container(&args, &arr)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    // Send message
    if (!dbus_connection_send(con_, msg, nullptr)) {
        fprintf(stderr, "Out Of Memory!\n");
        dbus_message_unref(msg);
        return false;
    }

    dbus_message_unref(msg);
    // dbus_connection_flush(con_);

    return true;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> DBus::read() {
    // Check if dbus is open
    if (con_ == nullptr)
        return CommunicationError::CONNECTION_CLOSED;

    // Read and remove message from dbus
    auto msg = dbus_connection_pop_message(con_);
    if (msg == nullptr)
        return CommunicationError::NO_DATA_AVAILABLE;

    // Check for correct type
    if (!dbus_message_is_method_call(msg, INTERFACE_NAME.c_str(), METHOD_NAME.c_str())) {
        dbus_message_unref(msg);
        return CommunicationError::INVALID_HEADER;
    }

    // Prepare read arguments
    DBusMessageIter args;
    if (!dbus_message_iter_init(msg, &args)) {
        fprintf(stderr, "Message has no arguments!\n");
        dbus_message_unref(msg);
        return CommunicationError::INVALID_DATA;
    }

    // Get object type
    auto path = &dbus_message_get_path(msg)[PATH_PREFIX.size()];
    auto type = static_cast<DataType>(std::stoul(path, nullptr));

    // Read id
    if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) {
        fprintf(stderr, "Argument is not an uint32!\n");
        dbus_message_unref(msg);
        return CommunicationError::INVALID_DATA;
    }

    std::uint32_t id;
    dbus_message_iter_get_basic(&args, &id);

    // Read timestamp
    if (!dbus_message_iter_next(&args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT64) {
        fprintf(stderr, "Argument is not an int64!\n");
        dbus_message_unref(msg);
        return CommunicationError::INVALID_DATA;
    }

    std::int64_t timestamp;
    dbus_message_iter_get_basic(&args, &timestamp);

    // Read object data
    if (!dbus_message_iter_next(&args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY
        || dbus_message_iter_get_element_type(&args) != DBUS_TYPE_BYTE) {
        fprintf(stderr, "Argument is not an byte array!\n");
        dbus_message_unref(msg);
        return CommunicationError::INVALID_DATA;
    }

    // Array data
    int size;
    std::byte *ptr;

    DBusMessageIter arr;
    dbus_message_iter_recurse(&args, &arr);
    dbus_message_iter_get_fixed_array(&arr, &ptr, &size);

    auto header = DataHeader(id, type, size, timestamp);

    // Handle each type differently
    switch (header.get_type()) {
        case DataType::INVALID: {
            dbus_message_unref(msg);
            return CommunicationError::INVALID_DATA;
        }

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto data = JavaSymbol::deserialize(ptr, size);
            dbus_message_unref(msg);

            if (!data)
                return CommunicationError::INVALID_DATA;

            return std::make_tuple(header, *data);
        }
    }

    // Unknown or invalid type
    dbus_message_unref(msg);
    return CommunicationError::UNKNOWN_DATA;
}

}
