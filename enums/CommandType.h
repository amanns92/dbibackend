//
// Created by Sebastian Amann on 16.09.24.
//

#ifndef COMMANDTYPE_H
#define COMMANDTYPE_H

#include <cstdint>  // For fixed-width integer types

// Enum for CommandType
enum class CommandType : uint8_t {
    REQUEST = 0,
    RESPONSE = 1,
    ACK = 2
};

#endif //COMMANDTYPE_H
