#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <netinet/in.h>
#include <sys/un.h>
}

#include "communication_handler.hpp"

namespace ipc {

class DatagramSocket : public ICommunicationHandler {
public:
    /**
     * Create a new unix domain Socket.
     *
     * @param path   Path to the socket.
     * @param server Whether this socket is the server.
     */
    DatagramSocket(std::string path, bool server);

    /**
     * Create a new internet domain Socket.
     *
     * @param address Internet address of the socket.
     * @param port    Port of the socket.
     * @param server  Whether this socket is the server.
     */
    DatagramSocket(std::string address, std::uint16_t port, bool server);

    /**
     * Destructor for this object to cleanup data and close socket.
     */
    ~DatagramSocket() override;

    bool open() override;

    bool close() override;

    bool is_open() const override;

    bool await_data() override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

private:
    /**
     * Build addresses for server and client.
     *
     * @return True, if addresses where build successfully.
     */
    bool build_address();

    /**
     * Create socket server.
     *
     * @return True, if socket server was created successfully.
     */
    bool create_server();

    /**
     * Create socket client.
     *
     * @return True, if socket client was created successfully.
     */
    bool create_client();

private:
    const std::tuple<std::string, std::optional<std::uint16_t>> parameters_;
    std::variant<sockaddr_un, sockaddr_in> address_;
    const bool server_;
    const bool unix_;
    int sfd_ = -1;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
