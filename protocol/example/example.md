```cpp
#pargma pack(push, 1)
struct chat_msg_header
{
  char compressflag;    // 0: not compressed, 1: compressed
  int32_t originsize;   // uncompressed size
  int32_t compresssize; // compressed size
  char reserved[16];    // 保留字段，用于将来扩展
};
#pragma pack(pop)
```
> 包体的内容长度无论是否设置了compressflag压缩标志，最后的实际长度都是originsize。

### 发送方示例代码
```cpp
//发送方组装包体的格式
std::string outbuf;
net::BinaryStreamWriter writeStream(&outbuf);
//消息类型
writeStream.writeInt32(msg_type_chat);
//消息序号
writeStream.writeInt32(m_seq);
//发送者ID
writeStream.Write(senderId);
//消息内容
writeStream.writeString(chatMsg);
//接受者ID
writeStream.writeInt32(receiverId);
writeStream.Flush();
```

**有一个小细节，即对于string类型的消息，在写入实际的字符串之间会先写入这个字符串的长度**
```cpp
//WriteString实际调用了WriteCString方法
bool BinaryStreamWriter::WriteString(const std::string& str)
{
  return WriteCString(str.c_str(), str.length());
}

//使用一个std::string存放二进制流，先将字符串的长度写入流中，再写入字符串本身的内容
bool BInaryStreamWriter::WriteCString(const char* str, size_t len)
{
  std::string buf;
  write7BitEncoded(len, buf); //对于字符串的长度，会根据其长度值缩成1~5字节
  m_data->append(buf); //m_data是outbuf的指针
  m_data->append(str, len);
  return true;
}
```
```cpp
//将一个4字节的整型数值压缩成1~5字节
void write7BitEncoded(uint32_t value, std::string& buf)
{
  do{
    unsigned char c = (unsigned char)(value & 0x7F);
    value >>= 7;
    if(value != 0){
      c |= 0x80;
    }
    buf.append(1, c);
  }while(value != 0);
}
```
```cpp
/*
在流的前4个字节处存放流数据的长度
存放长度使用的是网络字节序，即大端字节序
这4个字节在创建BinaryStreamWriter时被预留下来
*/
void BinaryStreamWriter::Flush()
{
  char* ptr = &(*m_data)[0];
  unsigned int ulen = htonl(m_data->length());
  memcpy(ptr, &ulen, sizeof(ulen));
}
```
```cpp
std::string outbuf;
net::BinaryStreamWriter writeStream(&outbuf);
```
以上代码调用了BinaryStreamWriter的构造函数
```cpp
enum{
  //4字节头长度
  BINARY_PACKLEN_LEN_2 = 4;
  CHECKSUM_LEN = 2;
}
BinaryStreamWriter::BinaryStreamWriter(std::string* data):
  m_data(data)
{
  m_data->clear();
  char str[BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN];
  m_data->append(str, sizeof(str));
}
```
```cpp
//p即包体流的指针
void TcpSession::sendPackage(const char* p, int32_t length)
{
  string srcbuf(p, length);
  string destbuf;
  //按需压缩
  if(m_bNeedCOmpress){
    if(!ZlibUtil::compressBuf(srcbuf, destbuf)){
      LOG_ERROR("compress buf failed");
      return;
    }
  }
  else{
    destbuf = srcbuf;
  }

  string strPackageData;
  chat_msg_header header;
  if(m_bNeedCOmpress){
    //设置压缩标志
    header.compressflag = PACKAGE_COMPRESSED;
    //设置压缩后的包体大小
    header.compresssize = destbuf.length();
  }
  else{
    header.compressflag = PACKAGE_UNCOMPRESSED;
  }

  //设置压缩前的包体大小
  header.originsize = srcbuf.length();

  //插入真正的包头
  strPackageData.append((const char*)&header, sizeof(header));
  strPackageData.append(destbuf);
}
```
> BinaryStreamWriter对浮点数的处理，是先将浮点数按一定的进度转换成字符串，然后将字符串写入流中

```cpp
//isNULL参数表示可以写入一个double类型的占位符
bool BinaryStreamWriter::writeDouble(double value, bool isNULL)
{
  char doublestr[128];
  if(isNULL == false){
    sprintf(doublestr, "%f", value);
    WriteCString(doublestr, strlen(doublestr));
  }
  else{
    WriteCString(doublestr, 0);
  }
}
```
```cpp
bool CRecvMsgThread::HandleMessage(const std::string& strMsg)
{
  //strMsg是包体流
  net::BinaryStreamReader reader(strMsg.c_str(), strMsg.length());
  //读取消息类型
  int32_t nMsgType;
  if(!readStream.ReadInt32(nMsgType)){
    LOG_ERROR("read msg type failed");
    return false;
  }
  //读取消息序列号
  if(!readStream.ReadInt64(nMsgSeq)){
    LOG_ERROR("read msg seq failed");
    return false;
  }

  //根据消息类型做处理
  switch(msgType){
    //聊天信息
    case msg_type_chat:
    {  //从流中读取发送者ID
      int32_t senderId;
      if(!readStream.ReadInt32(senderId)){
        LOG_ERROR("read sender id failed");
        break;
      }
      //从流中读取消息内容
      std::string chatMsg;
      size_t charMsgLength;
      if(!readStream.ReadString(&chatMsg, 0, charMsgLength)){
        LOG_ERROR("read chat msg failed");
        break;
      }
      //从流中读取接收者ID
      int32_t receiverId;
      if(!readStream.ReadInt32(receiverId)){
        LOG_ERROR("read receiver id failed");
        break;
      }
      //对聊天信息进行处理
      HandleChatMsg(senderId, receiverId, data);
    }
    break;
  }

  return false;
}
```