#pragma once

#include <string>
#include <vector>
#include <string.h>

namespace SimpleBLE {

using BluetoothAddress = std::string;

// IDEA: Extend BluetoothUUID to include a `uuid` function that
// returns the same string, but provides a homogeneous interface.
using BluetoothUUID = std::string;

// IDEA: Extend ByteArray to be constructed by a vector of bytes
// and pointers to uint8_t.
using ByteArray = std::vector<int8_t>;


enum class OperatingSystem {
    WINDOWS,
    MACOS,
    LINUX,
};

}  // namespace SimpleBLE
