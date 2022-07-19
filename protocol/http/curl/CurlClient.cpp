#include "CurlClient.h"
#include <iostream>

//有数据响应时的回调函数
size_t reqReply(void* ptr, size_t size, size_t nmemb, void* stream)
{
  std::string *str = (std::string*)stream;
  str->append((char*)ptr, size * nmemb);
  return size* nmemb;
}

bool CCurlClient::m_bGlobalInit = false;

CCurlClient::CCurlClient()
{}

CCurlClient::~CCurlClient()
{}

void CCurlClient::init()
{
  if (!m_bGlobalInit)
  {
    curl_global_init(CURL_GLOBAL_ALL);
    m_bGlobalInit = true;
  }
}

void CCurlClient::uninit()
{
  if (m_bGlobalInit)
  {
    curl_global_cleanup();
    m_bGlobalInit = false;
  }
}


bool CCurlClient::get(const char* url, const char* headers, std::string& response, bool autoRedirect/* = false*/, bool reserveHeaders/* = false*/, int64_t connTimeout/* = 1L*/, int64_t readTimeout/* = 5L*/)
{
  CURL* curl = curl_easy_init();
  if(curl == nullptr){
    return false;
  }

  //设置请求url
  curl_easy_setopt(curl, CURLOPT_URL, url);
  //不使用HTTPS，如果需要使用HTTPS，则将最后一个参数设置为true
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
  
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reqReply);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
  //禁用SIGALRM+sigsetjmp/soglongjmp的超时机制
  //采用其他的超时机制，因为该机制修改了一个全局变量，所以在多线程下可能出现问题
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connTimeout);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, readTimeout);

  //遇到HTTP 3xx状态码是否应该自动重定位
  if (autoRedirect){
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  }
  else{
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
  }

  if(reserveHeaders){
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
  }
  else{
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  }

  //添加自定义头信息
  if(headers != nullptr){
    struct curl_slist* chunk = NULL;
    chunk = curl_slist_append(chunk, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  }

  CURLcode res = curl_easy_perform(curl);

  curl_easy_cleanup(curl);

  return res == CURLcode::CURLE_OK;

}

bool CCurlClient::post(const char* url, const char* headers, const char* postParams, std::string& response, bool autoRedirect/* = false*/, bool reserveHeaders/* = false*/, int64_t connTimeout/* = 1L*/, int64_t readTimeout/* = 5L*/)
{
  CURL* curl = curl_easy_init();
  if(curl == nullptr){
    return false;
  }

  //设置请求请求方式为post
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  //设置请求url
  curl_easy_setopt(curl, CURLOPT_URL, url);
  //不使用HTTPS，如果需要使用HTTPS，则将最后一个参数设置为true
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
  
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reqReply);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
  //禁用SIGALRM+sigsetjmp/soglongjmp的超时机制
  //采用其他的超时机制，因为该机制修改了一个全局变量，所以在多线程下可能出现问题
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connTimeout);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, readTimeout);

  //遇到HTTP 3xx状态码是否应该自动重定位
  if (autoRedirect){
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  }
  else{
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
  }

  if(reserveHeaders){
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
  }
  else{
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
  }

  //添加自定义头信息
  if(headers != nullptr){
    struct curl_slist* chunk = NULL;
    chunk = curl_slist_append(chunk, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  }

  CURLcode res = curl_easy_perform(curl);

  curl_easy_cleanup(curl);

  return res == CURLcode::CURLE_OK;
}