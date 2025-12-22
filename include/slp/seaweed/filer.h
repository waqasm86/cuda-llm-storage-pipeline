#pragma once
#include <string>
#include <vector>

namespace slp::seaweed {

bool put_file(const std::string& filer_base,
              const std::string& path,
              const std::vector<uint8_t>& data);

std::vector<uint8_t> get_file(const std::string& filer_base,
                              const std::string& path);

} // namespace slp::seaweed
