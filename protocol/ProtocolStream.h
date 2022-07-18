#ifndef __PROTOCOL_STREAM_H__
#define __PROTOCOL_STREAM_H__

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <stdint.h>

//二进制协议的打包解包类，内部服务器之间的通信统一采用这些类
namespace net
{
  enum{
    TEXT_PACKLEN_LEN = 4,
    TEXT_PACKLEN_MAXLEN = 0xffff,
    BINARY_PACKLEN_LEN = 2,
    BINARY_PACKLEN_MAXLEN = 0xffff,

    TEXT_PACKLEN_LEN_2 = 6,
    TEXT_PACKLEN_MAXLEN_2 = 0xffffff,

    BINARY_PACKLEN_LEN_2 = 4,
    BINARY_PACKLEN_MAXLEN_2 = 0x10000000, //包最大长度是256MB, 足够用

    CHECKSUM_LEN = 2,
  };

  //计算校验和
  unsigned short checksum(const unsigned short* buffer, int size);
  //将一个4字节的整型数值压缩成1~5字节
  void write7BitEncoded(uint32_t value, std::string& buf);
  //将一个8字节的整型数值解压成1~10字节
  void write7BitEncoded(uint64_t value, std::string& buf);

  //将一个1~5字节的字符数组还原成一个4字节的整型数值
  void read7BitEncoded(const char* buf, uint32_t len, uint32_t& value);
    //将一个1~10字节的字符数组还原成一个8字节的整型数值
  void read7BitEncoded(const char* buf, uint32_t len, uint64_t& value);

  class BinaryStramReader final
  {
  public:
    BinaryStramReader(const char* ptr, size_t len);
    ~BinaryStramReader() = default;

    virtual const char* GetData() const;
    virtual size_t GetSize() const;

    bool IsEmpty() const;
    bool ReadString(std::string* str, size_t maxlen, size_t& outlen);
    bool ReadCString(char* str, size_t strlen, size_t& len);
    bool ReadInt32(int32_t& i);
    bool ReadInt64(int64_t& i);
    bool ReadShort(short& i);
    bool ReadChar(char& c);
    size_t ReadAll(char* szBuffer, size_t iLen) const;
    bool IsEnd() const;
    const char* GetCurrent() const {return m_cur;}
  
  public:
    bool ReadLength(size_t& len);
    bool ReadLengthWithoutOffset(size_t& headlen, size_t& outlen);
  
  private:
    BinaryStramReader(const BinaryStramReader&) = delete;
    BinaryStramReader& operator=(const BinaryStramReader&) = delete;
  
  private:
    const char* const m_ptr;
    const size_t m_len;
    const char* m_cur;
  };

  class BinaryStramWriter final
  {
  public:
    BinaryStramWriter(std::string* data);
    ~BinaryStramWriter() = default;

    virtual const char* GetData() const;
    virtual size_t GetSize() const;
    bool WriteCString(const char* str, size_t len);
    bool WriteString(const std::string& str);
    bool WriteDouble(double value, bool isNULL = false);
    bool WriteInt64(int64_t value, bool isNULL = false);
    bool WriteInt32(int32_t i, bool isNULL = false);
    bool WriteShort(short i, bool isNULL = false);
    bool WriteChar(char c, bool isNULL = false);
    size_t GetCurrentPos() const {return m_data->length();}
    void Flush();
    void Clear();

  private:
    BinaryStramWriter(const BinaryStramWriter&) = delete;
    BinaryStramWriter& operator=(const BinaryStramWriter&) = delete;

  private:
    std::string* m_data;

  };
}
#endif