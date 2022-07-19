#ifndef __CURL_CLIENT_H__
#define __CURL_CLIENT_H__

#include <string>
#include <curl/curl.h>

class CCurlClient final
{
public:
  CCurlClient();
  ~CCurlClient();

  CCurlClient(const CCurlClient&) = delete;
  CCurlClient& operator=(const CCurlClient&) = delete;

  /*
  初始化libcurl
  非线程安全函数，建议在程序初始化是调用一次，以免因多线程调用curl_easy_init而崩溃
  */
  static void init();

  /*
  反初始化libcurl
  非线程安全函数，建议在程序结束时调用一次
  */
  static void uninit();

  bool get(const char* url, const char* headers, std::string& response, bool autoRedirect = false, bool reserveHeaders = false, int64_t connTimeout = 1L, int64_t readTimeout = 5L);
  bool post(const char* url, const char* headers, const char* postParams, std::string& response, bool autoRedirect = false, bool reserveHeaders = false, int64_t connTimeout = 1L, int64_t readTimeout = 5L);

private:
  static bool m_bGlobalInit;
};




#endif