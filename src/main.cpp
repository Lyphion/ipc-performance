#include <iostream>
#include <iomanip>

#include "../include/data_header.hpp"
#include "../include/java_symbol.hpp"

void print_array(const char *data, int size) {
    std::ios_base::fmtflags f(std::cout.flags());

    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < size; ++i)
        std::cout << std::setw(sizeof(char) * 2) << (int) data[i];

    std::cout << std::endl;
    std::cout.flags(f);
}

int main() {
    auto header = ipc::DataHeader(
            0x01020304,
            ipc::DataType::JAVA_SYMBOL_LOOKUP,
            0x0708,
            0x090A0B0C0D0E0F00
    );

    auto buffer = new char[32];

    auto size = header.serialize(buffer, 32);
    std::cout << "Size: " << size << std::endl;
    std::cout << "Header: " << header << std::endl;

    print_array(buffer, 32);

    auto parsed_header = ipc::DataHeader::deserialize(buffer, 32);
    std::cout << "Parsed: " << parsed_header << std::endl;

    std::fill_n(buffer, 32, 0);

    auto symbol = ipc::JavaSymbol(0x1, 0x2, "ABC");
    size = symbol.serialize(buffer, 32);

    std::cout << "Size: " << size << std::endl;
    print_array(buffer, 32);

    auto parsed_symbol = ipc::JavaSymbol::deserialize(buffer, 32);

    delete[] buffer;

    return 0;
}
