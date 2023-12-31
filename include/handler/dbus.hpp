#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

extern "C" {
#include <dbus/dbus.h>
}

#include "communication_handler.hpp"

namespace ipc {

class DBus : public ICommunicationHandler {
public:
    /// Prefix for object type path.
    static const inline std::string PATH_PREFIX = "/ipc/object/";

    /// Interface name for communication.
    static const inline std::string INTERFACE_NAME = "ipc.object.type";

    /// Endpoint method name.
    static const inline std::string METHOD_NAME = "read";

    /**
     * Create a new dbus handler.
     *
     * @param name   Name of the dbus.
     * @param server Whether this dbus is the server.
     */
    DBus(std::string name, bool server);

    /**
     * Destructor for this object to cleanup data and close dbus.
     */
    ~DBus() override;

    bool open() override;

    bool close() override;

    bool is_open() const override;

    bool await_data() override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

    /**
     * Name of the DBus handler.
     */
    const std::string &name() const { return name_; }

    /**
     * Whether is dbus is the server.
     */
    bool server() const { return server_; }

private:
    /**
     * Create dbus server.
     *
     * @return True, if dbus server was created successfully.
     */
    bool create_server();

    /**
     * Create dbus client.
     *
     * @return True, if dbus client was created successfully.
     */
    bool create_client();

private:
    const std::string name_;
    const bool server_;

    DBusConnection *con_ = nullptr;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
