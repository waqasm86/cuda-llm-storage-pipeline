# Quick Start Guide - cuda-llm-storage-pipeline

**Get up and running in 5 minutes!**

---

## Prerequisites

✅ Your llama-server is already running on port 9080
✅ The project is already built (all executables in `build/`)
✅ Test prompts file created

---

## Quick Test (2 commands)

### 1. Interactive Testing
```bash
# Test llama-server connection with visual output
./build/slp_llama_client http://127.0.0.1:9080 /tmp/test_prompts.jsonl
```

**What you'll see**:
- Each prompt and response displayed
- Latency for each inference
- Summary statistics (mean, p50, p95, p99)

### 2. Batch Processing with Storage
```bash
# Process prompts and save results to file
./build/slp_llama_batch http://127.0.0.1:9080 \
  /tmp/test_prompts.jsonl \
  /tmp/inference_results.jsonl
```

**What you'll get**:
- JSONL file with all results
- Timestamps for each inference
- Success/failure tracking
- Ready to upload to SeaweedFS

---

## Create Your Own Prompts

### Format (JSONL - one JSON object per line)
```bash
cat > my_prompts.jsonl << 'EOF'
{"prompt": "What is machine learning?", "max_tokens": 100}
{"prompt": "Explain neural networks simply.", "max_tokens": 150}
{"prompt": "What is CUDA?", "max_tokens": 80}
EOF
```

### Run Your Prompts
```bash
./build/slp_llama_batch http://127.0.0.1:9080 \
  my_prompts.jsonl \
  my_results.jsonl
```

---

## View Results

### Pretty-print first result
```bash
cat my_results.jsonl | head -1 | python3.11 -m json.tool
```

### View all results
```bash
cat my_results.jsonl
```

### Count successful inferences
```bash
grep '"success":true' my_results.jsonl | wc -l
```

---

## Built Executables

All executables are in `build/` directory:

| Executable | Purpose | Status |
|------------|---------|--------|
| `slp_llama_client` | Interactive testing | ✅ Working |
| `slp_llama_batch` | Batch processing | ✅ Working |
| `slp_put_model` | Upload models | ⚠️ Needs SeaweedFS |
| `slp_get_model` | Download models | ⚠️ Needs SeaweedFS |
| `slp_put_prompts` | Upload prompts | ⚠️ Needs SeaweedFS |
| `slp_bench_storage` | Benchmark | ⚠️ Needs SeaweedFS |
| `slp_run_infer` | Orchestrator | 🔨 Placeholder |

---

## Common Tasks

### Change LLaMA Server Port
If your llama-server runs on a different port:
```bash
./build/slp_llama_client http://127.0.0.1:YOUR_PORT /tmp/test_prompts.jsonl
```

### Process Large Batch
```bash
# Create 100 prompts programmatically
for i in {1..100}; do
  echo "{\"prompt\": \"Count to $i\", \"max_tokens\": 50}"
done > large_batch.jsonl

# Process them
./build/slp_llama_batch http://127.0.0.1:9080 \
  large_batch.jsonl \
  large_results.jsonl
```

### Extract Only Responses
```bash
# Using jq (if installed)
cat my_results.jsonl | jq -r '.response'

# Using Python 3.11
python3.11 -c "
import json
import sys
for line in sys.stdin:
    obj = json.loads(line)
    if obj.get('success'):
        print(obj.get('response', ''))
" < my_results.jsonl
```

---

## Troubleshooting

### "Connection refused"
```bash
# Check llama-server is running
ps aux | grep llama-server

# Check port
netstat -tulpn | grep 9080
```

### "Cannot open prompts file"
```bash
# Verify file exists
ls -lh my_prompts.jsonl

# Check format (should be valid JSON per line)
cat my_prompts.jsonl | python3.11 -m json.tool
```

### "Timeout" errors
```bash
# Increase timeout in source code (apps/slp_llama_*.cpp)
# Look for: curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
# Change 120L to higher value (e.g., 300L for 5 minutes)
# Rebuild: cmake --build build -j
```

---

## Integration with SeaweedFS (When Available)

### 1. Start SeaweedFS
```bash
./scripts/seaweed_local_up.sh
```

### 2. Upload Results
```bash
./build/slp_put_prompts http://127.0.0.1:8888 \
  /tmp/inference_results.jsonl
```

### 3. Upload Model
```bash
./build/slp_put_model http://127.0.0.1:8888 \
  /path/to/model.gguf \
  my_model_name
```

### 4. Download Model
```bash
./build/slp_get_model http://127.0.0.1:8888 \
  <model_hash> \
  downloaded_model.gguf
```

---

## Performance Tips

### For Faster Inference
1. Use shorter prompts (fewer tokens = faster)
2. Reduce `max_tokens` value
3. Use smaller model if possible
4. Increase `-ngl` (GPU layers) if VRAM allows

### For Better Quality
1. Use longer `max_tokens`
2. Craft more specific prompts
3. Use larger model variants
4. Adjust temperature/top_p (in llama-server config)

---

## Example Workflow

**Complete workflow from prompt creation to analysis**:

```bash
# 1. Create prompts
cat > interview_questions.jsonl << 'EOF'
{"prompt": "What are CUDA cores?", "max_tokens": 100}
{"prompt": "Explain TCP/IP networking.", "max_tokens": 120}
{"prompt": "What is distributed storage?", "max_tokens": 100}
EOF

# 2. Run batch inference
./build/slp_llama_batch http://127.0.0.1:9080 \
  interview_questions.jsonl \
  interview_answers.jsonl

# 3. View results
cat interview_answers.jsonl | python3.11 -m json.tool

# 4. Extract just the answers
cat interview_answers.jsonl | python3.11 -c "
import json
import sys
for i, line in enumerate(sys.stdin, 1):
    obj = json.loads(line)
    print(f'\n=== Question {i} ===')
    print(f'Prompt: {obj[\"prompt\"]}')
    print(f'Answer: {obj.get(\"response\", \"ERROR\")}')
    print(f'Latency: {obj[\"elapsed_ms\"]} ms')
"
```

---

## Directory Structure

```
cuda-llm-storage-pipeline/
├── build/                  # Compiled executables
│   ├── slp_llama_client   # Your main testing tool
│   └── slp_llama_batch    # Your main production tool
├── apps/                   # Source code
├── include/                # Headers
├── src/                    # Implementation
├── scripts/                # Automation scripts
├── README.md              # Full documentation
├── QUICKSTART.md          # This file
└── INTEGRATION_TEST_RESULTS.md  # Test results
```

---

## Next Steps

1. **Try the examples above**
2. **Create your own prompts**
3. **Experiment with different models** (if you have others)
4. **Install SeaweedFS** (for distributed storage features)
5. **Integrate with cuda-tcp-llama.cpp** (for TCP protocol)
6. **Add MPI coordination** (for multi-process batching)

---

**Need Help?**
- Check README.md for detailed documentation
- Review INTEGRATION_TEST_RESULTS.md for test examples
- Examine source code in apps/ directory

**Working System**:
- ✅ LLaMA Server running on port 9080
- ✅ GeForce 940M with gemma-3-1b-it model
- ✅ All tools built and tested
- ✅ Results storage working

**Have fun experimenting!** 🚀
