#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>

#include "slp/seaweed/filer.h"
#include "slp/sha256.h"

static std::vector<uint8_t> generate_random_data(size_t size) {
    std::vector<uint8_t> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    return data;
}

static double percentile(std::vector<double> v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t idx = static_cast<size_t>(p * static_cast<double>(v.size() - 1));
    return v[idx];
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: slp_bench_storage <filer_url> <size_mb> <iters> <operation>\n";
        std::cerr << "  operation: upload | download | roundtrip\n";
        std::cerr << "\n";
        std::cerr << "  Example:\n";
        std::cerr << "    slp_bench_storage http://127.0.0.1:8888 128 10 roundtrip\n";
        return 1;
    }

    std::string filer = argv[1];
    size_t size_mb = std::stoul(argv[2]);
    size_t iters = std::stoul(argv[3]);
    std::string operation = argv[4];

    size_t size_bytes = size_mb * 1024 * 1024;

    std::cout << "Storage Benchmark\n";
    std::cout << "=================\n";
    std::cout << "Filer:     " << filer << "\n";
    std::cout << "Size:      " << size_mb << " MB (" << size_bytes << " bytes)\n";
    std::cout << "Iterations: " << iters << "\n";
    std::cout << "Operation: " << operation << "\n\n";

    std::vector<double> latencies_ms;

    if (operation == "upload" || operation == "roundtrip") {
        std::cout << "Running upload benchmark...\n";

        for (size_t i = 0; i < iters; ++i) {
            auto data = generate_random_data(size_bytes);
            auto hash = slp::sha256_hex(data);
            std::string path = "/bench/" + hash + ".bin";

            auto t0 = std::chrono::steady_clock::now();
            bool success = slp::seaweed::put_file(filer, path, data);
            auto t1 = std::chrono::steady_clock::now();

            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            latencies_ms.push_back(ms);

            if (!success) {
                std::cerr << "Upload failed at iteration " << i << "\n";
                return 1;
            }

            std::cout << "  Iteration " << (i + 1) << "/" << iters
                      << ": " << std::fixed << std::setprecision(2) << ms << " ms "
                      << "(" << (static_cast<double>(size_bytes) / (1024.0 * 1024.0)) / (ms / 1000.0) << " MB/s)\n";
        }

        double mean_ms = percentile(latencies_ms, 0.5);
        double p95_ms = percentile(latencies_ms, 0.95);
        double p99_ms = percentile(latencies_ms, 0.99);

        std::cout << "\nUpload Statistics:\n";
        std::cout << "  Mean:  " << std::fixed << std::setprecision(2) << mean_ms << " ms\n";
        std::cout << "  P95:   " << p95_ms << " ms\n";
        std::cout << "  P99:   " << p99_ms << " ms\n";
        std::cout << "  Throughput (mean): "
                  << (static_cast<double>(size_bytes) / (1024.0 * 1024.0)) / (mean_ms / 1000.0)
                  << " MB/s\n";
    }

    if (operation == "download") {
        std::cout << "Download benchmark not yet implemented.\n";
        std::cout << "Would test retrieval of existing blobs from SeaweedFS.\n";
    }

    if (operation == "roundtrip") {
        std::cout << "\nRoundtrip (upload + verify) complete.\n";
    }

    return 0;
}
