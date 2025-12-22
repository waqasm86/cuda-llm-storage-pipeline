#include <fstream>
#include <iostream>
#include <vector>

#include "slp/seaweed/filer.h"
#include "slp/sha256.h"

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open file");
    return {std::istreambuf_iterator<char>(f), {}};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: slp_put_prompts <filer_url> <prompts_file>\n";
        std::cerr << "  prompts_file should be JSONL or CSV format\n";
        return 1;
    }

    std::string filer = argv[1];
    std::string prompts_path = argv[2];

    try {
        auto bytes = read_file(prompts_path);
        auto hash = slp::sha256_hex(bytes);

        std::string obj_path = "/prompts/" + hash + ".jsonl";

        if (!slp::seaweed::put_file(filer, obj_path, bytes)) {
            std::cerr << "upload failed\n";
            return 1;
        }

        std::cout << "uploaded prompts hash=" << hash << " (" << bytes.size() << " bytes)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
