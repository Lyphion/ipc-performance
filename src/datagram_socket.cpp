#include "../include/datagram_socket.hpp"

#include <cassert>

extern "C" {
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include "../include/utility.hpp"

namespace ipc {

DatagramSocket::DatagramSocket(std::string path, bool server)
        : parameters_(std::make_tuple(std::move(path), std::nullopt)),
          server_(server), unix_(true) {}

DatagramSocket::DatagramSocket(std::string address, std::uint16_t port, bool server)
        : parameters_(std::make_tuple(std::move(address), port)),
          server_(server), unix_(false) {}

DatagramSocket::~DatagramSocket() {
    // Close socket if open
    if (sfd_ != -1) {
        DatagramSocket::close();
    }
}

bool DatagramSocket::open() {
    // Check if socket is already open
    if (sfd_ != -1)
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

bool DatagramSocket::create_server() {
    // Open socket depending on type
    if (unix_) {
        sfd_ = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (sfd_ == -1) {
            perror("DatagramSocket::create_server (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Remove old socket files
        remove(std::get<0>(address_).sun_path);

        // Bind socket for listing
        auto address = std::get<0>(address_);
        if (bind(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_un)) == -1) {
            perror("DatagramSocket::create_server (bind)");
            return false;
        }
    } else {
        sfd_ = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (sfd_ == -1) {
            perror("DatagramSocket::create_server (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Bind socket for listing
        auto address = std::get<1>(address_);
        if (bind(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_in)) == -1) {
            perror("DatagramSocket::create_server (bind)");
            return false;
        }
    }

    return true;
}

bool DatagramSocket::create_client() {
    // Open socket depending on type
    if (unix_) {
        sfd_ = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sfd_ == -1) {
            perror("DatagramSocket::create_client (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;
    } else {
        sfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sfd_ == -1) {
            perror("DatagramSocket::create_client (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;
    }

    return true;
}

bool DatagramSocket::close() {
    // Check if socket is already closed
    if (sfd_ == -1)
        return false;

    // Close socket
    ::close(sfd_);

    // Remove file only for unix
    if (unix_ && server_) {
        remove(std::get<0>(address_).sun_path);
    }

    sfd_ = -1;

    return true;
}

bool DatagramSocket::await_data() const {
    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Poll events and block until one is available
    auto res = poll(sfd_, -1);
    if (res == -1)
        perror("DatagramSocket::await_data (poll)");

    return res > 0;
}

bool DatagramSocket::has_data() const {
    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Poll events and block for 1ms
    auto res = poll(sfd_, 1);
    if (res == -1)
        perror("DatagramSocket::has_data (poll)");

    return res > 0;
}

bool DatagramSocket::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Serialize body
    auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1)
        return false;

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into socket
    ssize_t res;
    if (unix_) {
        auto address = reinterpret_cast<sockaddr *>(&std::get<0>(address_));
        res = sendto(sfd_, buffer_.data(), header_size + size, 0, address, sizeof(sockaddr_un));
    } else {
        auto address = reinterpret_cast<sockaddr *>(&std::get<1>(address_));
        res = sendto(sfd_, buffer_.data(), header_size + size, 0, address, sizeof(sockaddr_in));
    }

    if (res == -1)
        perror("DatagramSocket::write (sendto)");

    return res != -1;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> DatagramSocket::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if socket is open
    if (sfd_ == -1)
        return CommunicationError::CONNECTION_CLOSED;

    // Read data from socket
    auto result = recvfrom(sfd_, buffer_.data(), BUFFER_SIZE, 0, nullptr, nullptr);
    if (result == -1) {
        if (errno == EAGAIN)
            return CommunicationError::NO_DATA_AVAILABLE;

        perror("DatagramSocket::read (recvfrom)");
        return CommunicationError::READ_ERROR;
    }

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), result);
    if (!optional)
        return CommunicationError::INVALID_HEADER;

    auto header = *optional;
    assert(header.get_body_size() == result - header_size);

    auto body = deserialize_data_object(
            header.get_type(), &buffer_[header_size], header.get_body_size());

    if (std::holds_alternative<DataObject>(body)) {
        auto obj = std::get<DataObject>(body);
        return std::make_tuple(header, obj);
    } else {
        return std::get<CommunicationError>(body);
    }
}

bool DatagramSocket::build_address() {
    if (unix_) {
        auto path = std::get<0>(parameters_);

        // Construct unix server socket
        sockaddr_un s_addr{};
        s_addr.sun_family = AF_UNIX;
        strncpy(s_addr.sun_path, path.c_str(), sizeof(s_addr.sun_path) - 1);
        address_ = s_addr;
    } else {
        auto address = std::get<0>(parameters_);
        auto port = *std::get<1>(parameters_);

        // Construct internet server socket
        sockaddr_in s_addr{};
        s_addr.sin_family = AF_INET;
        s_addr.sin_addr.s_addr = INADDR_ANY;
        s_addr.sin_port = htons(port);

        if (!server_) {
            // Convert string address to IPv4 format
            if (inet_pton(AF_INET, address.c_str(), &s_addr.sin_addr) <= 0) {
                perror("DatagramSocket::build_address (inet_pton)");
                return false;
            }
        }

        address_ = s_addr;
    }

    return true;
}

}
