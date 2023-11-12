cmake_minimum_required(VERSION 3.22)
project(IPC VERSION 1.0)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 17)

# DBus header files
include_directories("/usr/include/dbus-1.0")
include_directories("/usr/lib/x86_64-linux-gnu/dbus-1.0/include")
link_libraries("dbus-1")

# Project files .hpp + .cpp
include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(IPC ${SOURCES})
