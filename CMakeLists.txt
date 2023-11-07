cmake_minimum_required(VERSION 3.22)
project(IPC VERSION 1.0)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 17)

add_executable(IPC src/main.cpp
        include/data_type.hpp
        include/data_object.hpp
        include/data_header.hpp
        src/data_header.cpp
        include/java_symbol.hpp
        src/java_symbol.cpp
        include/communication_handler.hpp
        include/fifo.hpp
        src/fifo.cpp
        include/utility.hpp
        src/utility.cpp
        include/datagram_socket.hpp
        src/datagram_socket.cpp
        include/message_queue.hpp
        src/message_queue.cpp
)
