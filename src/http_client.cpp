#include "slp/http_client.h"
#include <curl/curl.h>
#include <stdexcept>
#include <cstring>

namespace slp {

namespace {

// Callback for libcurl to write response data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* response = static_cast<std::vector<uint8_t>*>(userp);
    auto* data = static_cast<uint8_t*>(contents);
    response->insert(response->end(), data, data + total_size);
    return total_size;
}

// Callback for libcurl to read request data
size_t read_callback(char* buffer, size_t size, size_t nitems, void* userp) {
    auto* ctx = static_cast<std::pair<const std::vector<uint8_t>*, size_t>*>(userp);
    const auto& data = *ctx->first;
    size_t& offset = ctx->second;

    size_t max_copy = size * nitems;
    size_t remaining = data.size() - offset;
    size_t to_copy = std::min(max_copy, remaining);

    if (to_copy > 0) {
        std::memcpy(buffer, data.data() + offset, to_copy);
        offset += to_copy;
    }

    return to_copy;
}

} // anonymous namespace

HttpClient::HttpClient() {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
    }
}

HttpResponse HttpClient::get(const std::string& url) const {
    HttpResponse response;
    CURL* curl = static_cast<CURL*>(curl_);

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL GET failed: ") + curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);
    return response;
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::vector<uint8_t>& data,
                              const std::string& content_type) {
    HttpResponse response;
    CURL* curl = static_cast<CURL*>(curl_);

    // Context for read callback
    std::pair<const std::vector<uint8_t>*, size_t> read_ctx{&data, 0};

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &read_ctx);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(data.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 min for large uploads

    // Set content type header
    struct curl_slist* headers = nullptr;
    std::string content_type_header = "Content-Type: " + content_type;
    headers = curl_slist_append(headers, content_type_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL PUT failed: ") + curl_easy_strerror(res));
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);
    return response;
}

} // namespace slp
