#!/usr/bin/env bash
set -euo pipefail

BASE="/media/waqasm86/External2/seaweedfs"

MASTER_DIR="$BASE/master"
VOLUME_DIR="$BASE/volume"
FILER_DIR="$BASE/filer"

MASTER_PORT=9333
VOLUME_PORT=8080
FILER_PORT=8888

echo "[seaweed] Using base dir: $BASE"

mkdir -p "$MASTER_DIR" "$VOLUME_DIR" "$FILER_DIR"

echo "[seaweed] Starting master on :$MASTER_PORT"
nohup weed master \
  -ip=127.0.0.1 \
  -port=$MASTER_PORT \
  -mdir="$MASTER_DIR" \
  > "$MASTER_DIR/master.log" 2>&1 &

sleep 2

echo "[seaweed] Starting volume on :$VOLUME_PORT"
nohup weed volume \
  -ip=127.0.0.1 \
  -port=$VOLUME_PORT \
  -dir="$VOLUME_DIR" \
  -max=50 \
  -mserver=127.0.0.1:$MASTER_PORT \
  > "$VOLUME_DIR/volume.log" 2>&1 &

sleep 2

echo "[seaweed] Starting filer on :$FILER_PORT"
nohup weed filer \
  -ip=127.0.0.1 \
  -port=$FILER_PORT \
  -master=127.0.0.1:$MASTER_PORT \
  -dir="$FILER_DIR" \
  > "$FILER_DIR/filer.log" 2>&1 &

echo "[seaweed] All services started"
echo "  Master: http://127.0.0.1:$MASTER_PORT"
echo "  Volume: http://127.0.0.1:$VOLUME_PORT"
echo "  Filer : http://127.0.0.1:$FILER_PORT"

