#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <curl/curl.h>

// Batch inference tool that:
// 1. Processes prompts from JSONL file
// 2. Calls llama-server for each prompt
// 3. Saves results to output JSONL file (ready for SeaweedFS upload)

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
    while (pos < json.length() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;

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
    while (pos < json.length() && (std::isalnum(static_cast<unsigned char>(json[pos])) || json[pos] == '.' || json[pos] == '-')) {
        pos++;
    }
    return json.substr(start, pos - start);
}

std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

struct InferenceResult {
    std::string prompt;
    std::string content;
    int max_tokens;
    int64_t elapsed_us;
    bool success;
    std::string error;
    std::string timestamp;
};

InferenceResult call_llama_server(const std::string& url,
                                   const std::string& prompt,
                                   int max_tokens) {
    InferenceResult result;
    result.prompt = prompt;
    result.max_tokens = max_tokens;
    result.success = false;
    result.timestamp = get_iso_timestamp();

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize CURL";
        return result;
    }

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

    result.content = extract_json_field(response_body, "content");
    if (result.content.empty()) result.content = extract_json_field(response_body, "response");
    if (result.content.empty()) result.content = extract_json_field(response_body, "completion");
    if (result.content.empty()) result.content = extract_json_field(response_body, "text");

    if (!result.content.empty()) {
        result.success = true;
    } else {
        result.error = "Could not parse response";
        result.content = response_body.substr(0, 500);
    }

    return result;
}

std::string result_to_json(const InferenceResult& r) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"timestamp\":\"" << r.timestamp << "\",";
    oss << "\"prompt\":\"" << escape_json_string(r.prompt) << "\",";
    oss << "\"max_tokens\":" << r.max_tokens << ",";
    oss << "\"success\":" << (r.success ? "true" : "false") << ",";
    oss << "\"elapsed_ms\":" << std::fixed << std::setprecision(2) << (r.elapsed_us / 1000.0) << ",";

    if (r.success) {
        oss << "\"response\":\"" << escape_json_string(r.content) << "\"";
    } else {
        oss << "\"error\":\"" << escape_json_string(r.error) << "\"";
    }

    oss << "}";
    return oss.str();
}

void process_batch(const std::string& llama_url,
                   const std::string& prompts_file,
                   const std::string& output_file) {
    std::ifstream file(prompts_file);
    if (!file) {
        std::cerr << "Error: Cannot open prompts file: " << prompts_file << "\n";
        return;
    }

    std::ofstream outfile(output_file);
    if (!outfile) {
        std::cerr << "Error: Cannot create output file: " << output_file << "\n";
        return;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         Batch Inference - cuda-llm-storage-pipeline          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "LLaMA Server:  " << llama_url << "\n";
    std::cout << "Input:         " << prompts_file << "\n";
    std::cout << "Output:        " << output_file << "\n";
    std::cout << "Started:       " << get_iso_timestamp() << "\n\n";

    int prompt_num = 0;
    int success_count = 0;
    int failure_count = 0;
    std::string line;
    std::vector<int64_t> latencies;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        prompt_num++;

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

        std::cout << "[" << prompt_num << "] Processing: \"" << prompt_text.substr(0, 50)
                  << (prompt_text.length() > 50 ? "..." : "") << "\" ... " << std::flush;

        auto result = call_llama_server(llama_url, prompt_text, max_tokens);

        // Write result to output file
        outfile << result_to_json(result) << "\n";
        outfile.flush();

        if (result.success) {
            success_count++;
            latencies.push_back(result.elapsed_us);
            std::cout << "✓ (" << (result.elapsed_us / 1000.0) << " ms)\n";
        } else {
            failure_count++;
            std::cout << "✗ (" << result.error << ")\n";
        }
    }

    file.close();
    outfile.close();

    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      Batch Complete                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Total prompts:     " << prompt_num << "\n";
    std::cout << "Successful:        " << success_count << "\n";
    std::cout << "Failed:            " << failure_count << "\n";

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());

        int64_t total = 0;
        for (auto lat : latencies) total += lat;
        double mean_ms = (static_cast<double>(total) / static_cast<double>(latencies.size())) / 1000.0;
        double p50_ms = static_cast<double>(latencies[latencies.size() / 2]) / 1000.0;
        double p95_ms = static_cast<double>(latencies[static_cast<size_t>(static_cast<double>(latencies.size()) * 0.95)]) / 1000.0;
        double p99_ms = static_cast<double>(latencies[static_cast<size_t>(static_cast<double>(latencies.size()) * 0.99)]) / 1000.0;

        std::cout << "\nLatency Statistics:\n";
        std::cout << "  Mean:            " << std::fixed << std::setprecision(2) << mean_ms << " ms\n";
        std::cout << "  P50:             " << p50_ms << " ms\n";
        std::cout << "  P95:             " << p95_ms << " ms\n";
        std::cout << "  P99:             " << p99_ms << " ms\n";
    }

    std::cout << "\nResults saved to:  " << output_file << "\n";
    std::cout << "Next step:         Upload to SeaweedFS with slp_put_prompts\n\n";
}

} // anonymous namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: slp_llama_batch <llama_url> <prompts.jsonl> <output.jsonl>\n";
        std::cerr << "\n";
        std::cerr << "Example:\n";
        std::cerr << "  slp_llama_batch http://127.0.0.1:9080 prompts.jsonl results.jsonl\n";
        std::cerr << "\n";
        std::cerr << "Input format (JSONL):\n";
        std::cerr << "  {\"prompt\": \"What is AI?\", \"max_tokens\": 50}\n";
        std::cerr << "\n";
        std::cerr << "Output format (JSONL - ready for SeaweedFS upload):\n";
        std::cerr << "  {\"timestamp\":\"...\",\"prompt\":\"...\",\"success\":true,\"response\":\"...\"}\n";
        return 1;
    }

    try {
        process_batch(argv[1], argv[2], argv[3]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
