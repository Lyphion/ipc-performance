#include "ping.hpp"

namespace ipc {

int Ping::serialize(std::byte *, unsigned int) const {
    return 0;
}

std::optional<Ping> Ping::deserialize(const std::byte *, unsigned int) {
    return Ping();
}

std::ostream &operator<<(std::ostream &outs, const Ping &) {
    return outs << "()";
}

}
