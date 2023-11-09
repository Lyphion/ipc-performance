#include "../include/stream_socket.hpp"

#include <cassert>

extern "C" {
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include "../include/utility.hpp"

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
        close();
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
        sfd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_server (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Remove old socket files
        remove_file();

        // Bind socket for listing
        auto address = std::get<0>(address_);
        if (bind(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_un)) == -1) {
            perror("StreamSocket::create_server (bind)");
            return false;
        }
    } else {
        sfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd_ == -1) {
            perror("StreamSocket::create_server (socket)");
            return false;
        }

        // Build addresses for communication
        if (!build_address())
            return false;

        // Bind socket for listing
        auto address = std::get<1>(address_);
        if (bind(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_in)) == -1) {
            perror("StreamSocket::create_server (bind)");
            return false;
        }
    }

    // Listen for incoming socket connections
    if (listen(sfd_, BACKLOG) == -1) {
        perror("StreamSocket::create_server (listen)");
        return false;
    }

    // Accept new client
    auto res = ::accept(sfd_, nullptr, nullptr);
    if (res == -1) {
        perror("StreamSocket::create_server (accept)");
        return false;
    }

    cfd_ = res;

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
        auto address = std::get<0>(address_);
        if (connect(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_un)) == -1) {
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
        auto address = std::get<1>(address_);
        if (connect(sfd_, reinterpret_cast<sockaddr *>(&address), sizeof(sockaddr_in)) == -1) {
            perror("StreamSocket::create_client (connect)");
            return false;
        }
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
    if (unix_) {
        remove_file();
    }

    sfd_ = -1;
    cfd_ = -1;

    return true;
}

bool StreamSocket::accept() {
    // Check if socket is open
    if (sfd_ == -1)
        return false;

    // Accept new client
    auto res = ::accept(sfd_, nullptr, nullptr);
    if (res == -1) {
        perror("StreamSocket::accept (accept)");
    } else {
        if (cfd_ != -1)
            ::close(cfd_);
        cfd_ = res;
    }

    return res != -1;
}

bool StreamSocket::await_data() const {
    // Check if socket is open
    if (cfd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = cfd_;
    pfd.events = POLLIN;

    // Poll events and block until one is available
    auto res = ::poll(&pfd, 1, -1);
    if (res == -1)
        perror("StreamSocket::await_data (poll)");

    return res != -1;
}

bool StreamSocket::has_data() const {
    // Check if socket is open
    if (cfd_ == -1)
        return false;

    pollfd pfd{};
    pfd.fd = cfd_;
    pfd.events = POLLIN;

    // Poll events and block for 1ms
    auto res = ::poll(&pfd, 1, 1);
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
    std::uint32_t size = obj.serialize(&buffer_[header_size], BUFFER_SIZE - header_size);

    last_id_++;
    auto timestamp = get_timestamp();
    DataHeader header(last_id_, obj.get_type(), size, timestamp);

    // Serialize header
    header.serialize(buffer_.data(), header_size);

    // Write data into socket
    auto res = send(sfd_, buffer_.data(), header_size + size, 0);
    if (res == -1)
        perror("StreamSocket::write (write)");

    return res != -1;
}

std::optional<std::tuple<DataHeader, DataObject>> StreamSocket::read() {
    constexpr auto header_size = sizeof(DataHeader);

    // Check if pipe is open
    if (cfd_ == -1)
        return std::nullopt;

    // Read data from pipe
    auto result = recv(cfd_, buffer_.data(), header_size, 0);
    if (result == -1) {
        perror("StreamSocket::read (read)");
        return std::nullopt;
    }

    if (result == 0)
        return std::nullopt;

    assert(header_size == result);

    // Deserialize header
    auto optional = DataHeader::deserialize(buffer_.data(), result);
    if (!optional)
        return std::nullopt;

    auto header = *optional;

    result = recv(cfd_, buffer_.data(), header.get_body_size(), 0);
    if (result == -1) {
        perror("StreamSocket::read (read)");
        return std::nullopt;
    }

    if (result == 0)
        return std::nullopt;

    assert(header.get_body_size() == result);

    // Handle each type differently
    switch (header.get_type()) {
        case DataType::INVALID:
            break;

        case DataType::JAVA_SYMBOL_LOOKUP: {
            // Deserialize Java Symbols
            auto data = JavaSymbol::deserialize(buffer_.data(), result);
            if (!data)
                return std::nullopt;

            return std::make_tuple(header, *data);
        }
    }

    return std::nullopt;
}

bool StreamSocket::build_address() {
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
        s_addr.sin_port = htons(port);

        // Convert string address to IPv4 format
        if (inet_pton(AF_INET, address.c_str(), &s_addr.sin_addr) <= 0) {
            perror("StreamSocket::build_address (inet_pton)");
            return false;
        }

        address_ = s_addr;
    }

    return true;
}

void StreamSocket::remove_file() {
    if (server_) {
        remove(std::get<0>(address_).sun_path);
    }
}

}
