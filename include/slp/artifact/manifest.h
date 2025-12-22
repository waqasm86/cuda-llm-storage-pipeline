#pragma once
#include <string>

namespace slp::artifact {

struct Manifest {
  std::string sha256;
  uint64_t size_bytes = 0;
  std::string created_at;
  std::string original_name;

  std::string to_json() const;
};

} // namespace slp::artifact
