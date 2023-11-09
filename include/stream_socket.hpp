#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#include <netinet/in.h>
#include <sys/un.h>
}

#include "communication_handler.hpp"

namespace ipc {

class StreamSocket : public ICommunicationHandler {
public:
    /// Size of the buffer and limit of header and body combined.
    static constexpr short BUFFER_SIZE = 512;

    /// Maximum number of waiting socket connections.
    static constexpr unsigned char BACKLOG = 5;

    /**
     * Create a new unix domain Socket.
     *
     * @param path   Path to the socket.
     * @param server Whether this socket is the server.
     */
    StreamSocket(std::string path, bool server);

    /**
     * Create a new internet domain Socket.
     *
     * @param address Internet address of the socket.
     * @param port    Port of the socket.
     * @param server  Whether this socket is the server.
     */
    StreamSocket(std::string address, std::uint16_t port, bool server);

    /**
     * Destructor for this object to cleanup data and close socket.
     */
    ~StreamSocket();

    /**
     * Create a new socket.
     *
     * @return True, if socket was successfully opened.
     */
    bool open();

    /**
     * Close the current socket.
     *
     * @return True, if socket was successfully closed.
     */
    bool close();

    /**
     * Accept new clients.
     *
     * @return True, if a socket was accepted.
     * @remark Method will block until an event occurred.
     */
    bool accept();

    bool await_data() const override;

    bool has_data() const override;

    bool write(const IDataObject &obj) override;

    std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> read() override;

private:
    /**
     * Build addresses for server.
     *
     * @return True, if address was build successfully.
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

    /**
     * Remove temporary socket files.
     */
    void remove_file();

private:
    const std::tuple<std::string, std::optional<std::uint16_t>> parameters_;
    std::variant<sockaddr_un, sockaddr_in> address_;
    const bool server_;
    const bool unix_;
    int sfd_ = -1;
    int cfd_ = -1;

    std::uint32_t last_id_ = 0;
    std::array<std::byte, BUFFER_SIZE> buffer_{};
};

}
