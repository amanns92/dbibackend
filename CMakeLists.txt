cmake_minimum_required(VERSION 3.10)
project(dbibackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})

add_executable(dbibackend main.cpp
        src/UsbContext.cpp
        header/UsbContext.h
        enums/CommandId.h
        enums/CommandType.h
        src/Command.cpp
        header/Command.h)

target_link_libraries(dbibackend ${LIBUSB_LIBRARIES})
