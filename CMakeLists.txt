cmake_minimum_required(VERSION 3.22)
project(ipc VERSION 1.0)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

set(CMAKE_CXX_STANDARD 17)

# DBus header files
include_directories("/usr/include/dbus-1.0")
include_directories("/usr/lib/x86_64-linux-gnu/dbus-1.0/include")
link_libraries("dbus-1")

# Project files .cpp
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(ipc ${SOURCES})

# Project files .hpp
target_include_directories(ipc PRIVATE include)
