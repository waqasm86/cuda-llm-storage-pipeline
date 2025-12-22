#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>

#include "slp/seaweed/filer.h"
#include "slp/artifact/manifest.h"
#include "slp/sha256.h"

static void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot create file");
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: slp_get_model <filer_url> <model_hash> <output_path>\n";
        return 1;
    }

    std::string filer = argv[1];
    std::string hash = argv[2];
    std::string output_path = argv[3];

    try {
        // Download model from SeaweedFS
        std::string obj_path = "/models/" + hash + ".gguf";
        std::cout << "Downloading model from " << obj_path << "...\n";

        auto bytes = slp::seaweed::get_file(filer, obj_path);

        // Verify hash
        auto computed_hash = slp::sha256_hex(bytes);
        if (computed_hash != hash) {
            std::cerr << "Hash mismatch! Expected: " << hash << ", Got: " << computed_hash << "\n";
            return 1;
        }

        // Write to local file
        write_file(output_path, bytes);

        std::cout << "Downloaded model " << hash << " (" << bytes.size() << " bytes) to " << output_path << "\n";
        std::cout << "Hash verified: OK\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
