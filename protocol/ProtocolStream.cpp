#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "ProtocolStream.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <cassert>
#include <algorithm>

using namespace std;
namespace net
{
  //计算校验和
  unsigned short checksum(const unsigned short* buffer, int size)
  {
    unsigned int cksum = 0;
    while(size > 1){
      cksum += *buffer++;
      size -= sizeof(unsigned short);
    }

    if(size){
      cksum += *(unsigned char*)buffer;
    }

    //将32位数值变成16位数值
    while(cksum >>16){
      cksum = (cksum >> 16) + (cksum & 0xFFFF);
    }
    return (unsigned short)(~cksum);
  }

  //将一个4字节的整型数值压缩成1~5字节
  void write7BitEncoded(uint32_t value, std::string& buf)
  {
    do{
      unsigned char c = (unsigned char)(value & 0x7F);
      value >>= 7;
      if(value){
        c |= 0x80;
      }
      buf.append(1, c);
    }while(value);
  }

  //将一个8字节的整型数值解压成1~10字节
  void write7BitEncoded(uint64_t value, std::string& buf)
  {
    do{
      unsigned char c = (unsigned char)(value & 0x7F);
      value >>= 7;
      if(value){
        c |= 0x80;
      }
      buf.append(1, c);
    }while(value);
  }

  //将一个1~5字节的字符数组还原成一个4字节的整型数值
  void read7BitEncoded(const char* buf, uint32_t len, uint32_t& value)
  {
    char c;
    value = 0;
    int bitCount = 0;
    int index = 0;
    do{
      c = buf[index];
      uint32_t x = c & 0x7F;
      x <<= bitCount;
      value += x;
      bitCount += 7;
      ++index;
    }while(c & 0x80);
  }

  //将一个1~10字节的字符数组还原成一个8字节的整型数值
  void read7BitEncoded(const char* buf, uint32_t len, uint64_t& value)
  {
    char c;
    value = 0;
    int bitCount = 0;
    int index = 0;
    do{
      c = buf[index];
      uint64_t x = c & 0x7F;
      x <<= bitCount;
      value += x;
      bitCount += 7;
      ++index;
    }while(c & 0x80);
  }

  BinaryStreamReader::BinaryStreamReader(const char* ptr, size_t len)
    : m_ptr(ptr), m_len(len), m_cur(ptr)
  {
    m_cur += BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN;
  }

  bool BinaryStreamReader::IsEmpty() const
  {
    return m_len <= BINARY_PACKLEN_LEN_2;
  }

  size_t BinaryStreamReader::GetSize() const
  {
    return m_len;
  }

  bool BinaryStreamReader::ReadCString(char* buf, size_t strlen, /*out*/size_t &outlen)
  {
    size_t fieldlen;
    size_t headlen;
    if(!ReadLengthWithoutOffset(headlen, fieldlen)){
      return false;
    }
    if(fieldlen > strlen){
      return false;
    }

    //偏移量数据的位置
    m_cur += headlen;
    if(m_cur + fieldlen > m_ptr + m_len){
      outlen = 0;
      return false;
    }
    memcpy(buf, m_cur, fieldlen);
    outlen = fieldlen;
    m_cur += outlen;
    return true;
  }

  bool BinaryStreamReader::ReadString(string* str, size_t maxlen, size_t& outlen)
  {
    size_t fieldlen;
    size_t headlen;
    if(!ReadLengthWithoutOffset(headlen, fieldlen)){
      return false;
    }

    //用户缓冲区的内容长度不够
    if(maxlen != 0 && fieldlen > maxlen){
      return false;
    }

    //偏移量数据的位置
    m_cur += headlen;
    if(m_cur + fieldlen > m_ptr + m_len){
      outlen = 0;
      return false;
    }

    str->assign(m_cur, fieldlen);
    outlen = fieldlen;
    m_cur += outlen;
    return true;
  }

  bool BinaryStreamReader::ReadCCString(const char** str, size_t maxlen, size_t& outlen)
  {
    size_t fieldlen;
    size_t headlen;
    if(!ReadLengthWithoutOffset(headlen, fieldlen)){
      return false;
    }

    if(maxlen != 0 && fieldlen > maxlen){
      return false;
    }

    //偏移量数据的位置
    m_cur += headlen;
    if(m_cur + fieldlen > m_ptr + m_len){
      outlen = 0;
      return false;
    }

    *str = m_cur;
    outlen = fieldlen;
    m_cur += outlen;
    return true;
  }

  bool BinaryStreamReader::ReadInt32(int32_t& i)
  {
    const int VALUE_SIZE = sizeof(int32_t);
    if(m_cur + VALUE_SIZE > m_ptr + m_len){
      return false;
    }
    memcpy(&i, m_cur, VALUE_SIZE);
    i = ntohl(i);
    m_cur += VALUE_SIZE;
    return true;
  }

  bool BinaryStreamReader::ReadInt64(int64_t& i)
  {
    char int64str[128];
    size_t length;
    if(!ReadCString(int64str, sizeof(int64str), length)){
      return false;
    }

    i = atoll(int64str);

    return true;
  }

  bool BinaryStreamReader::ReadShort(short &i)
  {
    const int VALUE_SIZE = sizeof(short);
    if(m_cur + VALUE_SIZE > m_ptr + m_len){
      return false;
    }
    memcpy(&i, m_cur, VALUE_SIZE);
    i = ntohs(i);
    m_cur += VALUE_SIZE;
    return true;
  }

  bool BinaryStreamReader::ReadChar(char &i)
  {
    const int VALUE_SIZE = sizeof(char);
    if(m_cur + VALUE_SIZE > m_ptr + m_len){
      return false;
    }
    memcpy(&i, m_cur, VALUE_SIZE);
    m_cur += VALUE_SIZE;
    return true;
  }

  bool BinaryStreamReader::ReadLength(size_t& outlen)
  {
    size_t headlen;
    if(!ReadLengthWithoutOffset(headlen, outlen)){
      return false;
    }

    m_cur += headlen;
    return true;
  }

  bool BinaryStreamReader::ReadLengthWithoutOffset(size_t& headlen, size_t& outlen)
  {
    headlen = 0;
    const char* temp = m_cur;
    char buf[5];
    for(size_t i = 0; i < sizeof(buf); ++i){
      memcpy(buf +i, temp, sizeof(char));
      ++temp;
      headlen++;
      if(buf[i] & 0x80 == 0x00){
        break;
      }
    }

    if(m_cur + headlen > m_ptr + m_len){
      return false;
    }

    unsigned int value;
    read7BitEncoded(buf, headlen, value);
    outlen = value;
    return true;
  }

  bool BinaryStreamReader::IsEnd() const
  {
    return m_cur >= m_ptr + m_len;
  }

  const char* BinaryStreamReader::GetData() const
  {
    return m_ptr;
  }

  size_t BinaryStreamReader::ReadAll(char * szBuffer, size_t iLen) const
  {
    size_t iReadLen = min(iLen, m_len);
    memcpy(szBuffer, m_ptr, iReadLen);
    return iReadLen;
  }

  //==================== class BinaryStreamWriter implementation ====================
  BinaryStreamWriter::BinaryStreamWriter(string* data):
    m_data(data)
  {
    m_data->clear();
    char str[BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN];
    m_data->append(str, BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN);
  }

  bool BinaryStreamWriter::WriteCString(const char* str, size_t len)
  {
    std::string buf;
    write7BitEncoded(len, buf);
    m_data->append(buf);
    m_data->append(str, len);
    return true;
  }

  bool BinaryStreamWriter::WriteString(const string& str)
  {
    return WriteCString(str.c_str(), str.length());
  }

  const char* BinaryStreamWriter::GetData() const
  {
    return m_data->data();
  }

  size_t BinaryStreamWriter::GetSize() const
  {
    return m_data->length();
  }

  bool BinaryStreamWriter::WriteInt32(int32_t i, bool isNULL)
  {
    int32_t i2 = 999999999;
    if(isNULL == false){
      i2 = htonl(i);
    }
    m_data->append((char*)&i2, sizeof(int32_t));
    return true;
  }

  bool BinaryStreamWriter::WriteInt64(int64_t value, bool isNULL)
  {
    char int64str[128];
    if(isNULL == false){
#ifndef _WIN32
      sprintf(int64str, "%ld", value);
#else
      sprintf(int64str, "%lld", value);
#endif
      WriteCString(int64str, strlen(int64str));
    }
    else{
      WriteCString(int64str, 0);
    }
    return true;
  }

  bool BinaryStreamWriter::WriteShort(short i, bool isNULL)
  {
    short i2 = 0;
    if(isNULL == false){
      i2 = htons(i);
    }
    m_data->append((char*)&i2, sizeof(short));
    return true;
  }

  bool BinaryStreamWriter::WriteChar(char c, bool isNULL)
  {
    char c2 = 0;
    if(isNULL == false){
      c2 = c;
    }
    (*m_data) += c2;
    return true;
  }

  bool BinaryStreamWriter::WriteDouble(double d, bool isNULL)
  {
    char str[128];
    if(isNULL == false){
      sprintf(str, "%f", d);
      WriteCString(str, strlen(str));
    }
    else{
      WriteCString(str, 0);
    }
    return true;
  }

  void BinaryStreamWriter::Flush()
  {
    char* ptr = &(*m_data)[0];
    unsigned int ulen = htonl(m_data->length());
    memcpy(ptr, &ulen, sizeof(ulen));
  }

  void BinaryStreamWriter::Clear()
  {
    m_data->clear();
    char str[BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN];
    m_data->append(str, sizeof(str));
  }
}