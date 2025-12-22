#!/usr/bin/env bash
set -euo pipefail

BUILD=build/bin

$BUILD/slp_bench_storage \
  --master http://127.0.0.1:9333 \
  --filer  http://127.0.0.1:8888 \
  --size-mb 128 \
  --iters 10

