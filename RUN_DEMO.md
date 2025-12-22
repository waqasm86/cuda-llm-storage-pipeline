# Run the Complete Demo - Linux Commands

## Option 1: Automated Demo Script (Recommended)

**Single command to run everything:**

```bash
cd /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline
./scripts/demo_workflow.sh
```

This script will:
1. ✅ Create test prompts
2. ✅ Run batch inference with your llama-server (port 9080)
3. ✅ Save results locally
4. ✅ Copy results to SeaweedFS directory (`/media/waqasm86/External2/seaweedfs/uploads/llm_results/`)
5. ✅ Display summary statistics

---

## Option 2: Manual Step-by-Step Commands

### Step 1: Go to project directory
```bash
cd /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline
```

### Step 2: Create test prompts
```bash
cat > /tmp/my_prompts.jsonl << 'EOF'
{"prompt": "What is CUDA programming?", "max_tokens": 80}
{"prompt": "Explain distributed storage systems.", "max_tokens": 100}
{"prompt": "What are the benefits of quantized models?", "max_tokens": 90}
{"prompt": "Describe TCP/IP networking.", "max_tokens": 85}
{"prompt": "What is MPI in parallel computing?", "max_tokens": 95}
EOF
```

### Step 3: Run batch inference
```bash
./build/slp_llama_batch http://127.0.0.1:9080 \
  /tmp/my_prompts.jsonl \
  /tmp/my_results.jsonl
```

**Expected output:**
```
╔════════════════════════════════════════════════════════════════╗
║         Batch Inference - cuda-llm-storage-pipeline          ║
╚════════════════════════════════════════════════════════════════╝

[1] Processing: "What is CUDA programming?" ... ✓ (1234.56 ms)
[2] Processing: "Explain distributed storage..." ... ✓ (5678.90 ms)
...
Total prompts:     5
Successful:        5
```

### Step 4: View results
```bash
# Pretty print first result
cat /tmp/my_results.jsonl | head -1 | python3.11 -m json.tool

# View all results
cat /tmp/my_results.jsonl
```

### Step 5: Copy results to SeaweedFS directory
```bash
# Create results directory
mkdir -p /media/waqasm86/External2/seaweedfs/uploads/llm_results

# Copy results with timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
cp /tmp/my_results.jsonl \
  /media/waqasm86/External2/seaweedfs/uploads/llm_results/inference_results_${TIMESTAMP}.jsonl

cp /tmp/my_prompts.jsonl \
  /media/waqasm86/External2/seaweedfs/uploads/llm_results/prompts_${TIMESTAMP}.jsonl
```

### Step 6: Verify files in SeaweedFS directory
```bash
ls -lh /media/waqasm86/External2/seaweedfs/uploads/llm_results/
```

### Step 7: View stored results
```bash
# Find your latest results file
LATEST_RESULT=$(ls -t /media/waqasm86/External2/seaweedfs/uploads/llm_results/inference_results_*.jsonl | head -1)

# Display it
cat "$LATEST_RESULT" | python3.11 -m json.tool
```

---

## Quick Test Commands

### Test llama-server connection
```bash
cd /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline

./build/slp_llama_client http://127.0.0.1:9080 /tmp/test_prompts.jsonl
```

### Process one simple prompt
```bash
echo '{"prompt": "Hello, how are you?", "max_tokens": 30}' > /tmp/quick_test.jsonl

./build/slp_llama_batch http://127.0.0.1:9080 \
  /tmp/quick_test.jsonl \
  /tmp/quick_result.jsonl

cat /tmp/quick_result.jsonl | python3.11 -m json.tool
```

---

## Verify Everything is Working

```bash
# 1. Check llama-server is running
ps aux | grep llama-server

# 2. Check project executables exist
ls -lh /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline/build/slp_*

# 3. Check SeaweedFS directory exists
ls -ld /media/waqasm86/External2/seaweedfs/uploads/

# 4. Run the demo
cd /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline
./scripts/demo_workflow.sh
```

---

## What You'll See

✅ **During execution:**
- Progress for each prompt being processed
- Latency for each inference
- Success/failure status

✅ **In results file:**
- JSON formatted results with timestamps
- Each prompt and its response
- Latency measurements

✅ **In SeaweedFS directory:**
- Results files: `inference_results_YYYYMMDD_HHMMSS.jsonl`
- Prompts files: `prompts_YYYYMMDD_HHMMSS.jsonl`
- Organized in `/media/waqasm86/External2/seaweedfs/uploads/llm_results/`

---

## Copy-Paste Ready Command

**Just copy and paste this into your terminal:**

```bash
cd /media/waqasm86/External1/Project-CPP/Project-Nvidia/cuda-llm-storage-pipeline && ./scripts/demo_workflow.sh
```

**That's it!** The script does everything automatically.
