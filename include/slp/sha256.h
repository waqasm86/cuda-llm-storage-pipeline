#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace slp {

// Compute SHA256 hash of data and return as hex string
std::string sha256_hex(const std::vector<uint8_t>& data);

// Compute SHA256 hash of data and return as raw bytes
std::vector<uint8_t> sha256_raw(const std::vector<uint8_t>& data);

} // namespace slp
