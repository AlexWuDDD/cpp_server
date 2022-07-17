# 网络通信协议设计
## 流式协议
协议的内容时流水一样的字节流，内容与内容之间没有明确的分界标志，需要我们人为地给这些内容划分边界。

>网络通信时，如何解决粘包、丢包、或者包乱序问题？

如果使用TCP进行通信，则在大多数场景下时不存在丢包或者包乱序的问题。因为TCP通信是可靠的通信方式，TCP栈通过序列号和包重传确认机制保证数据包的有序和一定被正确发送到目的地；
如果使用UDP进行通信，且不允许少量丢包，就要自己在UDP的基础上实现类似TCP这种有序且可靠的传输机制（例如RTP、RUDP）。

所以将拆解后，就只剩下如何解决粘包的问题了。

**什么是粘包?**
粘包就是连续向对端发送两个或两个以上的数据包，对端在一次收取中收到的数据包数量可能大于1个，当大于1个时，可能是几个（包括一个）包加上某个包的部分，或者干脆几个完整的包在一起。当然，也可能收到的数据只是一个包的部分，这种情况一般也叫做半包。

因为TCP是流式数据格式，所以解决思路还是从收到的数据中把包与包的边界区分出来。
如何区分？一般有三种方法：
1. 固定包长的数据包。
2. 以特定的字符(串)为包的结束标志。
3. 包头+包体格式 

> 对于一名合格的后端开发者来说，根据应用场景设计合理的网络通信协议是对其的基本要求。

#### 在设计网络通信协议时，要考虑哪些因素？ 
- 协议的演化
- 协议的分类
  - 文办协议
  - 二进制协议
- 协议设计工具
  - IDL - Interface Description Language
  - Goolge Protocol Buffers -> protoc就是一个IDL工具

## 设计通信协议时的注意事项
### 1.字节对齐
```cpp
#pargma pack(push 1)
struct userinfo
{
  short version;
  int32_t cmd;
  char gender;
  char name[8];
  int32_t age;
};
#pargma pack(pop)
```
> #pargma pack(push 1)告诉编译器将接下来的所有结构体的每一个字段都按1字节对齐，即去除所有padding字节。这样做是为了时内存更加紧凑即节省存储空间。

### 2.显示地指定整型字段地长度
int32_t, int64_t
### 3.涉及浮点数要考虑进度问题
在网络传输时将浮点数数值方法相应的倍数，变成整数或者转换为字符串来传输
### 4.大小端问题
在设计协议格式时，如果在协议中存在整型字段，则建议使用同一字节序。
通常的做法是在进行网络传输时将所有整型都转换为网络字节序（大端编码， Big Endian）
### 5.协议与自动升级功能
无论版本如何迭代，一定要保证自动升级协议的新旧兼容

## 包分片
应用层对包的拆分。
```cpp
//与客户端交互协议包头
#pragma pack(push 1)
typedef struct tagNtPkgHead
{
  unsigned char bSTartFlag;         //协议包起始标志，固定为0xFF
  unsigned char bVer;               //协议版本号
  unsigned char bEncryptFlag;       //加密标志，0表示不加密，1表示加密
  unsigned char bFrag;              //包分片标志，0表示不分片，1表示分片
  unsigned short wLen;              //总包长
  unsigned short wCmd;              //命令号
  unsigned short wSeq;              //包序号
  unsigned short wCrc;              //Crc16校验码
  unsigned int dwSID;               //会话ID
  unsigned short wTotal;            //有包分片时的分片总数
  unsigned short wCurseq;            //有包分片时的当前分片序号，从0开始，无分片时也为0
}NtPkgHead， *PNtPkgHead;
#pragma pack(pop)