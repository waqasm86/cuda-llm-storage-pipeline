#include "slp/seaweed/filer.h"
#include "slp/http_client.h"
#include <stdexcept>

namespace slp::seaweed {

bool put_file(const std::string& filer_base,
              const std::string& path,
              const std::vector<uint8_t>& data) {
    try {
        HttpClient client;
        std::string url = filer_base + path;
        auto response = client.put(url, data, "application/octet-stream");

        // SeaweedFS returns 201 (Created) or 200 (OK) on success
        return response.status == 201 || response.status == 200;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<uint8_t> get_file(const std::string& filer_base,
                              const std::string& path) {
    HttpClient client;
    std::string url = filer_base + path;
    auto response = client.get(url);

    if (response.status != 200) {
        throw std::runtime_error("Failed to get file: HTTP " + std::to_string(response.status));
    }

    return response.body;
}

} // namespace slp::seaweed
