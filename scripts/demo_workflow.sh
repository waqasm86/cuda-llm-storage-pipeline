#!/usr/bin/env bash
set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     cuda-llm-storage-pipeline Demo Workflow                  ║"
echo "║     Integration: llama.cpp + SeaweedFS                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="/media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline"
SEAWEEDFS_DIR="/media/waqasm86/External2/seaweedfs"
LLAMA_URL="http://127.0.0.1:9080"

echo -e "${BLUE}[Step 1]${NC} Creating test prompts file..."
cat > /tmp/demo_prompts.jsonl << 'EOF'
{"prompt": "What is CUDA programming?", "max_tokens": 80}
{"prompt": "Explain distributed storage systems.", "max_tokens": 100}
{"prompt": "What are the benefits of quantized models?", "max_tokens": 90}
{"prompt": "Describe TCP/IP networking.", "max_tokens": 85}
{"prompt": "What is MPI in parallel computing?", "max_tokens": 95}
EOF

echo -e "${GREEN}✓${NC} Created /tmp/demo_prompts.jsonl with 5 prompts"
echo ""

echo -e "${BLUE}[Step 2]${NC} Running batch inference with llama-server..."
echo "         (This will take ~30-60 seconds depending on your GPU)"
echo ""

cd "$PROJECT_ROOT"
./build/slp_llama_batch "$LLAMA_URL" \
  /tmp/demo_prompts.jsonl \
  /tmp/demo_results.jsonl

echo ""
echo -e "${GREEN}✓${NC} Batch inference completed!"
echo ""

echo -e "${BLUE}[Step 3]${NC} Verifying results file..."
if [ -f /tmp/demo_results.jsonl ]; then
    RESULT_COUNT=$(wc -l < /tmp/demo_results.jsonl)
    echo -e "${GREEN}✓${NC} Results file created with $RESULT_COUNT entries"
    echo ""
    echo "First result (formatted):"
    head -1 /tmp/demo_results.jsonl | python3.11 -m json.tool 2>/dev/null || head -1 /tmp/demo_results.jsonl
else
    echo -e "${YELLOW}⚠${NC} Results file not found!"
    exit 1
fi

echo ""
echo -e "${BLUE}[Step 4]${NC} Copying results to SeaweedFS directory..."

# Create results directory in SeaweedFS storage
mkdir -p "$SEAWEEDFS_DIR/uploads/llm_results"

# Copy with timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_FILE="$SEAWEEDFS_DIR/uploads/llm_results/inference_results_${TIMESTAMP}.jsonl"

cp /tmp/demo_results.jsonl "$RESULT_FILE"

echo -e "${GREEN}✓${NC} Results copied to:"
echo "   $RESULT_FILE"
echo ""

# Also copy prompts
PROMPTS_FILE="$SEAWEEDFS_DIR/uploads/llm_results/prompts_${TIMESTAMP}.jsonl"
cp /tmp/demo_prompts.jsonl "$PROMPTS_FILE"

echo -e "${GREEN}✓${NC} Prompts copied to:"
echo "   $PROMPTS_FILE"
echo ""

echo -e "${BLUE}[Step 5]${NC} Verifying files in SeaweedFS directory..."
echo ""
echo "Contents of $SEAWEEDFS_DIR/uploads/llm_results/:"
ls -lh "$SEAWEEDFS_DIR/uploads/llm_results/"
echo ""

echo -e "${BLUE}[Step 6]${NC} Generating summary statistics..."
echo ""

python3.11 << 'PYTHON_SCRIPT'
import json
import sys

results_file = '/tmp/demo_results.jsonl'

total = 0
success = 0
failed = 0
latencies = []

print("=" * 70)
print("INFERENCE SUMMARY")
print("=" * 70)
print()

with open(results_file, 'r') as f:
    for i, line in enumerate(f, 1):
        obj = json.loads(line)
        total += 1

        if obj.get('success'):
            success += 1
            latencies.append(obj['elapsed_ms'])
            print(f"[{i}] ✓ SUCCESS")
            print(f"    Prompt: {obj['prompt'][:60]}...")
            print(f"    Latency: {obj['elapsed_ms']:.2f} ms")
            print(f"    Response: {obj['response'][:80]}...")
            print()
        else:
            failed += 1
            print(f"[{i}] ✗ FAILED")
            print(f"    Prompt: {obj['prompt'][:60]}...")
            print(f"    Error: {obj.get('error', 'Unknown')}")
            print()

print("=" * 70)
print(f"Total prompts:     {total}")
print(f"Successful:        {success}")
print(f"Failed:            {failed}")

if latencies:
    latencies.sort()
    mean = sum(latencies) / len(latencies)
    p50 = latencies[len(latencies) // 2]
    p95 = latencies[int(len(latencies) * 0.95)]
    p99 = latencies[int(len(latencies) * 0.99)]

    print()
    print("Latency Statistics:")
    print(f"  Mean:  {mean:.2f} ms")
    print(f"  P50:   {p50:.2f} ms")
    print(f"  P95:   {p95:.2f} ms")
    print(f"  P99:   {p99:.2f} ms")

print("=" * 70)
PYTHON_SCRIPT

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                   WORKFLOW COMPLETE! ✓                        ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}Summary:${NC}"
echo "  • Prompts processed by llama-server (port 9080)"
echo "  • Results saved locally: /tmp/demo_results.jsonl"
echo "  • Results stored in SeaweedFS: $RESULT_FILE"
echo "  • Prompts stored in SeaweedFS: $PROMPTS_FILE"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "  • View results: cat $RESULT_FILE | python3.11 -m json.tool"
echo "  • List all results: ls -lh $SEAWEEDFS_DIR/uploads/llm_results/"
echo "  • Run again: ./scripts/demo_workflow.sh"
echo ""
