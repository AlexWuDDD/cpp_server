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
  
}