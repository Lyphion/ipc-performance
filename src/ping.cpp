#include "../include/ping.hpp"

namespace ipc {

int Ping::serialize(std::byte *, unsigned int) const {
    return true;
}

std::optional<Ping> Ping::deserialize(const std::byte *, unsigned int) {
    return Ping();
}

std::ostream &operator<<(std::ostream &outs, const Ping &) {
    return outs << "()";
}

}
