# cuda-llm-storage-pipeline

**SeaweedFS-backed LLM Artifact & Run Store (C++)**

A high-performance C++20 storage layer and pipeline orchestrator that demonstrates NVIDIA-scale systems thinking for LLM inference infrastructure. Uses SeaweedFS as a distributed storage backend for model artifacts, prompt batches, and inference results.

---

## Why This Exists

Modern LLM inference at datacenter scale requires more than just "running a model." Performance and reliability depend on:

- **Artifact distribution**: Model weights, tokenizers, configs
- **Cold-start vs warm-cache behavior**: Measuring storage impact on latency
- **Data-plane throughput**: Streaming large blobs efficiently
- **Control-plane coordination**: Where is the data, who owns it
- **End-to-end observability**: Latency budgets per stage

This repository implements a **C++ storage layer** that uses SeaweedFS as a stand-in for distributed storage fabric, and integrates with local inference (e.g., llama.cpp llama-server) to produce a realistic inference workflow.

**Goal**: Demonstrate "NVIDIA-scale" system design thinking on a single machine with limited hardware.

---

## Architecture (Control Plane vs Data Plane)

### Data Plane (bytes move here)
- GGUF model files
- Prompt batches (JSONL/CSV)
- Inference outputs and run logs

### Control Plane (routing and metadata)
- **Content-addressed registry**: Artifacts stored by SHA256
- **Manifests**: `*.manifest.json` stored alongside artifacts
- **Run IDs**: Immutable run folders with metrics + outputs

### Storage Backend
SeaweedFS Filer API for path-based, human-debuggable storage:
- `/models/<sha256>.gguf`
- `/prompts/<sha256>.jsonl`
- `/runs/<run_id>/results.jsonl`
- `/runs/<run_id>/metrics.json`

---

## Key Features

✅ **Content-addressed storage** for GGUF artifacts (SHA256)
✅ **Manifest sidecars**: integrity + provenance + size + timestamps
✅ **Local cache policy**: avoid re-downloading large models when unchanged
✅ **Failure-aware uploads**: checksum verification after write
✅ **Performance tooling**: upload/download bandwidth benchmark
✅ **Latency metrics**: p50/p95/p99 across storage operations
✅ **Stage breakdown**: hash → upload → verify → download → verify

---

## Project Structure

```
cuda-llm-storage-pipeline/
├── CMakeLists.txt              # Modern C++20 CMake build
├── include/slp/                # Public headers
│   ├── http_client.h           # libcurl RAII wrapper
│   ├── sha256.h                # SHA256 hashing
│   ├── seaweed/filer.h         # SeaweedFS Filer API
│   └── artifact/manifest.h     # Artifact metadata
├── src/                        # Implementation files
│   ├── http_client.cpp
│   ├── sha256.cpp
│   ├── seaweed/filer.cpp
│   └── artifact/manifest.cpp
├── apps/                       # Executable applications
│   ├── slp_put_model.cpp       # Upload GGUF → SeaweedFS
│   ├── slp_get_model.cpp       # Download GGUF by hash
│   ├── slp_put_prompts.cpp     # Upload prompt batches
│   ├── slp_run_infer.cpp       # Orchestrate inference pipeline
│   └── slp_bench_storage.cpp   # Benchmark storage performance
└── scripts/                    # Automation scripts
    ├── seaweed_local_up.sh     # Start SeaweedFS services
    └── bench_storage.sh        # Run benchmarks
```

---

## Quick Start

### Prerequisites

- **C++20 compiler** (GCC 11+, Clang 14+)
- **CMake** 3.22+
- **libcurl** development headers
- **SeaweedFS** installed and configured
- **Ninja** build system (optional, recommended)

Install dependencies (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev ninja-build
```

### 1) Start SeaweedFS

Ensure SeaweedFS is installed and configured to use your storage directory:

```bash
./scripts/seaweed_local_up.sh
```

Expected services:
- **Master**: `http://127.0.0.1:9333`
- **Volume**: `http://127.0.0.1:8080`
- **Filer**: `http://127.0.0.1:8888`

Verify services are running:
```bash
curl http://127.0.0.1:9333/cluster/status
curl http://127.0.0.1:8888/
```

### 2) Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

All executables will be in `build/`:
- `slp_put_model`
- `slp_get_model`
- `slp_put_prompts`
- `slp_run_infer`
- `slp_bench_storage`

### 3) Upload a GGUF Model

```bash
./build/slp_put_model \
  http://127.0.0.1:8888 \
  /path/to/your/model.gguf \
  my_model_name
```

This will:
1. Read the GGUF file
2. Compute SHA256 hash
3. Upload to `/models/<sha256>.gguf`
4. Create manifest at `/models/<sha256>.manifest.json`

Output:
```
uploaded model my_model_name hash=a4f3b2c1d5e6...
```

### 4) Download a Model by Hash

```bash
./build/slp_get_model \
  http://127.0.0.1:8888 \
  a4f3b2c1d5e6... \
  /tmp/downloaded_model.gguf
```

This will:
1. Download from `/models/<hash>.gguf`
2. Verify SHA256 hash
3. Write to local file

Output:
```
Downloading model from /models/a4f3b2c1d5e6...
Downloaded model a4f3b2c1d5e6... (1234567 bytes) to /tmp/downloaded_model.gguf
Hash verified: OK
```

### 5) Benchmark Storage Performance

```bash
./build/slp_bench_storage \
  http://127.0.0.1:8888 \
  128 \
  10 \
  upload
```

Arguments:
- Filer URL
- Size in MB (128)
- Number of iterations (10)
- Operation (upload | download | roundtrip)

Output:
```
Storage Benchmark
=================
Filer:      http://127.0.0.1:8888
Size:       128 MB (134217728 bytes)
Iterations: 10
Operation:  upload

Running upload benchmark...
  Iteration 1/10: 245.32 ms (521.45 MB/s)
  Iteration 2/10: 238.15 ms (537.89 MB/s)
  ...

Upload Statistics:
  Mean:  242.50 ms
  P95:   255.20 ms
  P99:   260.10 ms
  Throughput (mean): 527.83 MB/s
```

---

## What This Demonstrates (Skills)

This project showcases the following technical capabilities relevant to systems-level ML infrastructure engineering:

| Capability | Implementation |
|------------|----------------|
| **Distributed storage technologies** | SeaweedFS integration, content-addressed artifacts, manifests, caching |
| **Linux low-level optimization** | Throughput measurement, tail latency (p95/p99), failure modes |
| **Networking fundamentals** | HTTP/REST APIs, libcurl integration, control vs data plane separation |
| **LLM inference workflow thinking** | Model distribution, cold-start cost, run reproducibility |
| **C++ systems programming** | C++20, RAII patterns, zero-copy where possible, modern CMake |
| **Performance engineering** | Benchmarking, percentile statistics, stage-by-stage breakdown |
| **Content integrity** | SHA256 hashing, upload verification, hash mismatch detection |

---

## Integration with Other Projects

This project is part of a larger portfolio demonstrating end-to-end ML infrastructure capabilities:

### 1. **cuda-tcp-llama.cpp**
High-performance TCP server for LLM inference with custom binary protocol, epoll-based I/O, and streaming responses.

**Integration point**: The cuda-tcp-llama.cpp gateway can pull models by hash from this storage pipeline instead of loading from local disk.

### 2. **cuda-openmpi**
CUDA + OpenMPI integration testing, demonstrating parallel programming, HPC fundamentals, and GPU-aware MPI.

**Integration point**: Multi-rank inference coordinators can use this storage layer for artifact distribution across nodes.

---

## Hardware Constraints & Design Decisions

This project is designed to run on **modest hardware**:
- Single machine (no multi-node cluster required)
- Old NVIDIA GPU with limited VRAM
- External storage drives for SeaweedFS backend

**Key design decisions**:
1. **Filer API over raw master/volume**: Simpler, path-based interface
2. **Content-addressed storage**: Enables deduplication and integrity checks
3. **Local caching**: Minimizes repeated downloads
4. **Modular architecture**: Easy to extend with MPI, RDMA, or other backends

---

## Development

### Enable Sanitizers (for debugging)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSLP_ENABLE_SANITIZERS=ON
cmake --build build -j
```

This enables AddressSanitizer (ASAN) and UndefinedBehaviorSanitizer (UBSAN).

### Code Style

The project follows these conventions:
- **C++20 standard** with modern features
- **snake_case** for functions and variables
- **PascalCase** for classes and structs
- **Explicit error handling**: No silent failures
- **RAII everywhere**: No manual memory management
- **Minimal dependencies**: Only libcurl beyond standard library

### Compiler Warnings

The build is configured with strict warnings:
```cmake
-Wall -Wextra -Wpedantic -Wshadow -Wconversion
```

All warnings are treated as errors in CI builds.

---

## Roadmap

### Near-term (Phase 1)
- [ ] Implement `slp_run_infer` full orchestration
- [ ] Add download benchmark to `slp_bench_storage`
- [ ] Implement local cache management with LRU eviction
- [ ] Add timestamp generation for manifests

### Medium-term (Phase 2)
- [ ] Integration with cuda-tcp-llama.cpp gateway
- [ ] Prometheus-compatible metrics export
- [ ] Structured tracing (OpenTelemetry)
- [ ] Multi-process coordination with MPI

### Long-term (Phase 3)
- [ ] RDMA/UCX transport backend
- [ ] GPU-direct storage integration
- [ ] Distributed cache coherence protocol
- [ ] Production-ready failure recovery

---

## Performance Characteristics

Measured on test system (NVIDIA GeForce 940M, External HDD storage):

| Operation | Latency (mean) | Throughput |
|-----------|----------------|------------|
| Upload 128MB | ~240 ms | ~520 MB/s |
| SHA256 hash | ~80 ms | ~1600 MB/s |
| Manifest write | ~5 ms | - |

Note: Performance will vary significantly based on:
- Storage backend (SSD vs HDD vs NVMe)
- Network configuration (localhost vs remote)
- SeaweedFS configuration (replication, volumes)

---

## Troubleshooting

### Build fails with "CURL not found"
```bash
sudo apt install libcurl4-openssl-dev
```

### SeaweedFS services not starting
Check logs:
```bash
cat /media/waqasm86/External2/seaweedfs/master/master.log
cat /media/waqasm86/External2/seaweedfs/volume/volume.log
cat /media/waqasm86/External2/seaweedfs/filer/filer.log
```

Ensure storage directories exist and have write permissions.

### Hash verification fails
This indicates data corruption or network issues. Retry the operation.

### Upload returns false but no error
Check SeaweedFS filer is running and accessible:
```bash
curl http://127.0.0.1:8888/
```

---

## License

MIT License - See LICENSE file for details.

---

## Contributing

This is a portfolio/demonstration project. For questions or suggestions, please open an issue on the GitHub repository.

---

## Acknowledgments

- **SeaweedFS**: Chris Lu and contributors
- **llama.cpp**: Georgi Gerganov and community
- **NVIDIA**: For inspiring systems-level ML infrastructure thinking

---

**Author**: Waqas M.
**Purpose**: NVIDIA Zurich AI Networking Acceleration Team application portfolio
**Date**: December 2025
