#include "handler/stream_socket.hpp"

#include <utility>

extern "C" {
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include "utility.hpp"

namespace ipc {

StreamSocket::StreamSocket(std::string path, bool server)
        : parameters_(std::make_tuple(std::move(path), std::nullopt)),
          server_(server), unix_(true) {}

StreamSocket::StreamSocket(std::string address, std::uint16_t port, bool server)
        : parameters_(std::make_tuple(std::move(address), port)),
          server_(server), unix_(false) {}

StreamSocket::~StreamSocket() {
    // Close socket if open
    if (sfd_ != -1) {
        StreamSocket::close();
    }
}

bool StreamSocket::open() {
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

bool StreamSocket::create_server() {
    // Open socket depending on type
    if (unix_) {
        sfd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_server (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Remove old socket files
        remove(std::get<0>(address_).sun_path);

        // Bind socket for listing
        const auto address = std::get<0>(address_);
        if (bind(sfd_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_un)) == -1) {
            perror("StreamSocket::create_server (bind)");
            return false;
        }
    } else {
        sfd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_server (socket)");
            return false;
        }

        int option = 1;
        setsockopt(sfd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        // Build addresses for communication
        if (!build_address())
            return false;

        // Bind socket for listing
        const auto address = std::get<1>(address_);
        if (bind(sfd_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_in)) == -1) {
            perror("StreamSocket::create_server (bind)");
            return false;
        }
    }

    // Listen for incoming socket connections
    if (listen(sfd_, BACKLOG) == -1) {
        perror("StreamSocket::create_server (listen)");
        return false;
    }

    return true;
}

bool StreamSocket::create_client() {
    // Open socket depending on type
    if (unix_) {
        sfd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_client (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Connect to server
        const auto address = std::get<0>(address_);
        if (connect(sfd_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_un)) == -1) {
            perror("StreamSocket::create_client (connect)");
            return false;
        }
    } else {
        sfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_client (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Connect to server
        const auto address = std::get<1>(address_);
        if (connect(sfd_, reinterpret_cast<const sockaddr *>(&address), sizeof(sockaddr_in)) == -1) {
            perror("StreamSocket::create_client (connect)");
            return false;
        }

        // Disable buffering and send data immediately (Disable Nagle algorithm)
        // int value = 1;
        // setsockopt(sfd_, IPPROTO_TCP, TCP_NODELAY,  &value, sizeof(value));
    }

    return true;
}

bool StreamSocket::close() {
    // Check if socket is already closed
    if (sfd_ == -1)
        return false;

    // Close socket
    ::close(sfd_);

    if (cfd_ != -1)
        ::close(cfd_);

    // Remove file only for unix
    if (unix_ && server_) {
        remove(std::get<0>(address_).sun_path);
    }

    sfd_ = -1;
    cfd_ = -1;

    return true;
}

bool StreamSocket::is_open() const {
    return sfd_ != -1;
}

bool StreamSocket::accept() {
    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Close old client socket
    if (cfd_ != -1) {
        ::close(cfd_);
        cfd_ = -1;
    }

    // Check if new clients are available
    auto res = poll(sfd_, WAIT_TIME);
    if (res == -1) {
        perror("StreamSocket::accept (poll)");
        return false;
    }

    // No new clients available
    if (res == 0)
        return false;

    // Accept new client
    res = ::accept4(sfd_, nullptr, nullptr, SOCK_NONBLOCK);
    if (res == -1) {
        perror("StreamSocket::accept (accept)");
        return false;
    }

    cfd_ = res;
    return true;
}

bool StreamSocket::await_data() {
    // Check if socket is open
    if (cfd_ == -1) {
        if (!accept())
            return false;
    }

    // Poll events and block until one is available
    const auto res = poll(cfd_, WAIT_TIME);
    if (res == -1)
        perror("StreamSocket::await_data (poll)");

    return res > 0;
}

bool StreamSocket::has_data() const {
    // Check if socket is open
    if (cfd_ == -1)
        return false;

    // Poll events and block for 1ms
    const auto res = poll(cfd_, 1);
    if (res == -1)
        perror("StreamSocket::has_data (poll)");

    return res > 0;
}

bool StreamSocket::write(const IDataObject &obj) {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Serialize body
    const auto timestamp = get_timestamp();
    const auto size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);
    if (size == -1)
        return false;

    last_id_++;
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into socket
    const auto res = send(sfd_, buffer_.data(), header_size + size, 0);
    if (res == -1)
        perror("StreamSocket::write (send)");

    return res != -1;
}

std::variant<std::tuple<DataHeader, DataObject>, CommunicationError> StreamSocket::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if pipe is open
    if (cfd_ == -1)
        return CommunicationError::CONNECTION_CLOSED;

    // Read data from pipe
    long result;
    unsigned int amount = 0;
    do {
        result = recv(cfd_, &buffer_[amount], header_size - amount, 0);
        if (result == -1) {
            if (errno == EAGAIN) {
                // Some part of the header was already read -> waiting for rest
                if (amount > 0)
                    continue;

                // Nothing read yet -> no data available
                return CommunicationError::NO_DATA_AVAILABLE;
            }

            perror("StreamSocket::read (recv)");
            return CommunicationError::READ_ERROR;
        }

        // We expect at least 1 byte every time we read
        if (result == 0)
            return CommunicationError::CONNECTION_CLOSED;

        amount += result;
    } while (amount < header_size);

    // Deserialize header
    const auto optional = DataHeader::deserialize(buffer_.data(), header_size);
    if (!optional)
        return CommunicationError::INVALID_HEADER;

    const auto header = *optional;

    // Read data from pipe
    amount = 0;
    while (amount < header.get_body_size()) {
        result = recv(cfd_, &buffer_[amount], header.get_body_size() - amount, 0);
        if (result == -1) {
            if (errno == EAGAIN)
                continue;

            perror("StreamSocket::read (recv)");
            return CommunicationError::READ_ERROR;
        }

        // We expect at least 1 byte every time we read
        if (result == 0)
            return CommunicationError::CONNECTION_CLOSED;

        amount += result;
    }

    const auto body = deserialize_data_object(header.get_type(), buffer_.data(), header.get_body_size());

    if (std::holds_alternative<DataObject>(body)) {
        const auto obj = std::get<DataObject>(body);
        return std::make_tuple(header, obj);
    } else {
        return std::get<CommunicationError>(body);
    }
}

bool StreamSocket::build_address() {
    if (unix_) {
        const auto path = std::get<0>(parameters_);

        // Construct unix server socket
        sockaddr_un s_addr{};
        s_addr.sun_family = AF_UNIX;
        strncpy(s_addr.sun_path, path.c_str(), sizeof(s_addr.sun_path) - 1);
        address_ = s_addr;
    } else {
        const auto address = std::get<0>(parameters_);
        const auto port = *std::get<1>(parameters_);

        // Construct internet server socket
        sockaddr_in s_addr{};
        s_addr.sin_family = AF_INET;
        s_addr.sin_addr.s_addr = INADDR_ANY;
        s_addr.sin_port = htons(port);

        if (!server_) {
            // Convert string address to IPv4 format
            if (inet_pton(AF_INET, address.c_str(), &s_addr.sin_addr) <= 0) {
                perror("StreamSocket::build_address (inet_pton)");
                return false;
            }
        }

        address_ = s_addr;
    }

    return true;
}

}
