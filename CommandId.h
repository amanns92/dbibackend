//
// Created by Sebastian Amann on 16.09.24.
//

#ifndef COMMANDID_H
#define COMMANDID_H

#include <cstdint>  // For fixed-width integer types

// Enum for CommandID
enum class CommandID : uint8_t {
    EXIT = 0,
    LIST_DEPRECATED = 1,
    FILE_RANGE = 2,
    LIST = 3
};

#endif //COMMANDID_H
