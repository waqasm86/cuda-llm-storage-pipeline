#pragma once
#include <string>
#include <vector>

namespace slp {

struct HttpResponse {
  long status = 0;
  std::vector<uint8_t> body;
};

class HttpClient {
public:
  HttpClient();
  ~HttpClient();

  HttpResponse get(const std::string& url) const;
  HttpResponse put(const std::string& url,
                   const std::vector<uint8_t>& data,
                   const std::string& content_type);

private:
  void* curl_;
};

} // namespace slp
