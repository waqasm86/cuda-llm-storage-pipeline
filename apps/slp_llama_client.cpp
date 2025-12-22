#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <curl/curl.h>

// Simple llama-server client without SeaweedFS dependency
// This demonstrates integration with your running llama-server on port 9080

namespace {

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::string escape_json_string(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 32) {
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

std::string extract_json_field(const std::string& json, const std::string& field) {
    std::string search = "\"" + field + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos += search.length();
    while (pos < json.length() && std::isspace(json[pos])) pos++;

    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end = start;
        while (end < json.length() && json[end] != '"') {
            if (json[end] == '\\' && end + 1 < json.length()) end += 2;
            else end++;
        }
        return json.substr(start, end - start);
    }

    size_t start = pos;
    while (pos < json.length() && (std::isalnum(json[pos]) || json[pos] == '.' || json[pos] == '-')) {
        pos++;
    }
    return json.substr(start, pos - start);
}

struct InferenceResult {
    std::string content;
    int64_t elapsed_us;
    bool success;
    std::string error;
};

InferenceResult call_llama_server(const std::string& url,
                                   const std::string& prompt,
                                   int max_tokens) {
    InferenceResult result;
    result.success = false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize CURL";
        return result;
    }

    // Build JSON request
    std::ostringstream json_body;
    json_body << "{\n";
    json_body << "  \"prompt\": \"" << escape_json_string(prompt) << "\",\n";
    json_body << "  \"n_predict\": " << max_tokens << ",\n";
    json_body << "  \"stream\": false\n";
    json_body << "}";

    std::string request_body = json_body.str();
    std::string response_body;

    std::string endpoint = url + "/completion";

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    auto t0 = std::chrono::steady_clock::now();
    CURLcode res = curl_easy_perform(curl);
    auto t1 = std::chrono::steady_clock::now();

    result.elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result.error = std::string("CURL error: ") + curl_easy_strerror(res);
        return result;
    }

    // Try to extract content from response
    result.content = extract_json_field(response_body, "content");
    if (result.content.empty()) {
        result.content = extract_json_field(response_body, "response");
    }
    if (result.content.empty()) {
        result.content = extract_json_field(response_body, "completion");
    }
    if (result.content.empty()) {
        result.content = extract_json_field(response_body, "text");
    }

    if (!result.content.empty()) {
        result.success = true;
    } else {
        result.error = "Could not parse response (no recognized content field)";
        result.content = response_body; // Return raw response for debugging
    }

    return result;
}

void process_prompts_file(const std::string& llama_url, const std::string& prompts_file) {
    std::ifstream file(prompts_file);
    if (!file) {
        std::cerr << "Error: Cannot open prompts file: " << prompts_file << "\n";
        return;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  LLaMA Server Integration Test - cuda-llm-storage-pipeline   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "LLaMA Server: " << llama_url << "\n";
    std::cout << "Prompts File: " << prompts_file << "\n\n";

    int prompt_num = 0;
    std::string line;
    std::vector<int64_t> latencies;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        prompt_num++;

        // Simple JSONL parsing
        std::string prompt_text;
        int max_tokens = 50;

        size_t prompt_pos = line.find("\"prompt\":");
        if (prompt_pos != std::string::npos) {
            size_t start = line.find('"', prompt_pos + 9) + 1;
            size_t end = start;
            while (end < line.length() && line[end] != '"') {
                if (line[end] == '\\' && end + 1 < line.length()) end += 2;
                else end++;
            }
            prompt_text = line.substr(start, end - start);
        }

        size_t tokens_pos = line.find("\"max_tokens\":");
        if (tokens_pos != std::string::npos) {
            max_tokens = std::stoi(extract_json_field(line, "max_tokens"));
        }

        if (prompt_text.empty()) {
            std::cerr << "Warning: Could not parse prompt from line: " << line << "\n";
            continue;
        }

        std::cout << "─────────────────────────────────────────────────────────────────\n";
        std::cout << "Prompt #" << prompt_num << ": \"" << prompt_text << "\"\n";
        std::cout << "Max Tokens: " << max_tokens << "\n";
        std::cout << "─────────────────────────────────────────────────────────────────\n";

        auto result = call_llama_server(llama_url, prompt_text, max_tokens);

        if (result.success) {
            latencies.push_back(result.elapsed_us);

            std::cout << "\n✓ Success!\n";
            std::cout << "Response:\n";
            std::cout << "┌─────────────────────────────────────────────────────────────┐\n";
            std::cout << "│ " << result.content << "\n";
            std::cout << "└─────────────────────────────────────────────────────────────┘\n";
            std::cout << "\nLatency: " << (result.elapsed_us / 1000.0) << " ms\n\n";
        } else {
            std::cout << "\n✗ Failed: " << result.error << "\n";
            std::cout << "Raw response: " << result.content.substr(0, 200) << "...\n\n";
        }
    }

    if (!latencies.empty()) {
        std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                      Summary Statistics                       ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

        int64_t total = 0;
        for (auto lat : latencies) total += lat;
        double mean_ms = (total / static_cast<double>(latencies.size())) / 1000.0;

        std::sort(latencies.begin(), latencies.end());
        double p50_ms = latencies[latencies.size() / 2] / 1000.0;
        double p95_ms = latencies[static_cast<size_t>(latencies.size() * 0.95)] / 1000.0;
        double p99_ms = latencies[static_cast<size_t>(latencies.size() * 0.99)] / 1000.0;

        std::cout << "Prompts processed: " << latencies.size() << "\n";
        std::cout << "Mean latency:      " << std::fixed << std::setprecision(2) << mean_ms << " ms\n";
        std::cout << "P50 latency:       " << p50_ms << " ms\n";
        std::cout << "P95 latency:       " << p95_ms << " ms\n";
        std::cout << "P99 latency:       " << p99_ms << " ms\n\n";
    }
}

} // anonymous namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: slp_llama_client <llama_url> <prompts_file.jsonl>\n";
        std::cerr << "\n";
        std::cerr << "Example:\n";
        std::cerr << "  slp_llama_client http://127.0.0.1:9080 /tmp/test_prompts.jsonl\n";
        std::cerr << "\n";
        std::cerr << "Prompts file format (JSONL):\n";
        std::cerr << "  {\"prompt\": \"What is AI?\", \"max_tokens\": 50}\n";
        std::cerr << "  {\"prompt\": \"Explain ML.\", \"max_tokens\": 100}\n";
        return 1;
    }

    std::string llama_url = argv[1];
    std::string prompts_file = argv[2];

    try {
        process_prompts_file(llama_url, prompts_file);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
