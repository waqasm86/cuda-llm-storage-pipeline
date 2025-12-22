#include <fstream>
#include <iostream>
#include <vector>

#include "slp/seaweed/filer.h"
#include "slp/artifact/manifest.h"
#include "slp/sha256.h"

static std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open file");
  return {std::istreambuf_iterator<char>(f), {}};
}

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "usage: slp_put_model <filer_url> <model.gguf> <model_name>\n";
    return 1;
  }

  std::string filer = argv[1];
  std::string model_path = argv[2];
  std::string model_name = argv[3];

  auto bytes = read_file(model_path);
  auto hash = slp::sha256_hex(bytes);

  std::string obj_path = "/models/" + hash + ".gguf";

  if (!slp::seaweed::put_file(filer, obj_path, bytes)) {
    std::cerr << "upload failed\n";
    return 1;
  }

  slp::artifact::Manifest m;
  m.sha256 = hash;
  m.size_bytes = bytes.size();
  m.original_name = model_name;

  std::string manifest_path = "/models/" + hash + ".manifest.json";
  auto manifest_bytes =
      std::vector<uint8_t>(m.to_json().begin(), m.to_json().end());

  slp::seaweed::put_file(filer, manifest_path, manifest_bytes);

  std::cout << "uploaded model " << model_name
            << " hash=" << hash << "\n";
}
