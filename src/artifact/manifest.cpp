#include "slp/artifact/manifest.h"
#include <sstream>
#include <iomanip>

namespace slp::artifact {

std::string Manifest::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"sha256\": \"" << sha256 << "\",\n";
    oss << "  \"size_bytes\": " << size_bytes << ",\n";
    oss << "  \"created_at\": \"" << created_at << "\",\n";
    oss << "  \"original_name\": \"" << original_name << "\"\n";
    oss << "}";
    return oss.str();
}

} // namespace slp::artifact
