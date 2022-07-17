# Note
## Linux set socket non-block
---
```cpp
int oldSocketFlag = fcntl(sockfd, F_GETFL, 0);
int newSocketFlag = oldSocketFlag | O_NONBLOCK;
fcntl(sockfd, F_SETFL, newSocketFlag);
```
OR
```cpp
int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
```
---
```cpp
sockleb_t addrlen = sizeof(clientaddr);
int clientfd = accept4(sockfd, (struct sockaddr *)&clientaddr, &addrlen, SOCK_NONBLOCK);
```
equal
```cpp
sockleb_t addrlen = sizeof(clientaddr);
int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &addrlen);
if(clientfd != -1){
  int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
  int newSocketFlag = oldSocketFlag | O_NONBLOCK;
  fcntl(clientfd, F_SETFL, newSocketFlag);
}
```

## socket send/recv non-block
|returned n|meaning|
|----|----|
|>0|send/recv success|
|0|peer close|
|<0|error, EINTR, TCP Window is too small/recv buffer has nothing|

```cpp
### Not recommend
```cpp
int n = send(sockfd, buf, buf_length, 0);
if(n== buf_length){
  printf("send data successfully\n");
}
```
### Recommend
```cpp
bool SendData(const char *buf, int buf_length)
{
  //sent bytes count
  int sent_bytes = 0;
  int ret = 0;
  while(true){
    ret = send(m_hSocket, buf + sent_bytes, buf_length - sent_bytes, 0);
    if(ret == -1){
      if(errno ==EWOULDBLOCK){
        //if cannot send, should buffer the unsent data
        break;
      }
      else if(errno == EINTR){
        continue;
      }
      else{
        printf("send error: %s\n", strerror(errno));
        return false;
      }
    }
    else if(ret == 0){
      printf("send error: %s\n", strerror(errno));
      return false;
    }
    else{
      sent_bytes += ret;
      if(sent_bytes == buf_length){
        break;
      }
    }
  }
  return true;
}
```
|return value and errno|send()|recv()|OS|
|----|----|----|----|
|return -1, errno is EWOULDBLOCK/EAGAIN|TCP Window is too small|no data in kernel buffer|Linux|
|return -1, errno is EINTR|Interrupted by signal, need to retry|Interrupted by signal, need to retry|Linux|
|return -1, errno does not belong above|something wrong|something wrong|Linux|

## effect of sending 0 bytes
when send() return 0:
- peer close and we are trying to send
- send 0 byte data

But recv() will return 0 when peer close

## Linux SIGPIPE
```cpp
signal(SIGPIPE, SIG_IGN);
```

## Linux poll
```cpp
struct pollfd{
  int fd;           /*fd which is waiting for checking*/
  short events;     /*intrested events*/
  short revents;    /*gotten events after checking */
};
```

## Linux epoll
```cpp
int epoll_create(int size); //size > 0
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```
- epfd = epoll_create(1024);
- op = EPOLL_CTL_ADD | EPOLL_CTL_MOD | EPOLL_CTL_DEL;
- fd = sockfd;
- event = struct epoll_event{
  uint32_t events; //EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP
  epoll_data_t data; //user defined data
};

```cpp
typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;
```

```cpp
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```
```cpp
while(true){
  epoll_event events[1024];
  int n = epoll_wait(epfd, events, 1024, 1000);
  if(n < 0){
    if(errno ==EINTER){
      continue;
    }
    else{
      printf("epoll_wait error: %s\n", strerror(errno));
      break;
    }
  }
  else if(n == 0){
    printf("epoll_wait timeout\n");
  }
  else{
    for(int i = 0; i < n; i++){
      if(events[i].events & EPOLLIN){
        //read data
      }
      else if(events[i].events & EPOLLOUT){
        //write data
      }
      else if(events[i].events & EPOLLERR){
        //error
      }
      else if(events[i].events & EPOLLHUP){
        //peer close
      }
    }
  }
}
```
EPOLLET: Edge Trigger, ET 
EPOLLLT: Level Trigger, LT
if fd has data,   we can think it is in high level
if fd has nodata, we can think it is in low level

if fd can be writen,     we can think it is in high level
if fd can not be writen, we can think it is in low level

LT is triggered:
1. low->high
2. keep high

ET is triggered:
1. low->high

```cpp
//ET read data
bool TcpSession::RecvEtMode()
{
  char buff[256];
  while(true){
    int nRecv = ::recv(m_clientfd, buff, sizeof(buff), 0);
    if(nRecv == -1){
      //must call recv in loop, until report error and error number is EWOULDBLOCK, which means read all the data
      if(errno == EWOULDBLOCK){
        return true;
      }
      else if(errno == EINTR){
        continue;
      }
      else{
        printf("recv error: %s\n", strerror(errno));
        return false;
      }
    }
    else if(nRecv == 0){
      printf("peer close\n");
      return false;
    }
    else{
      m_inputBuffer.Append(buff, (size_t)nRecv);
    }
  }

  return true;
}
```
在目前主流的网络库中，发送数据的逻辑都不是上面所说的依赖于写时间触发，在写事件触发时去发送数据。这种做法不好。

#### 总结一下
1. 在LT模式下，读时间触发后可以按需收取想要的字节数，不用吧本次接收的数据收取干净；在LT模式下，读事件时必须把数据收取干净，因为我们不一定再有机会收取数据了，即使有机会，也可能因为没有及时处理上次没读完的数据，造成客户端响应延迟。
2. 在LT模式下，不需要写事件时一定要及时移除，避免不必要地触发且浪费CPU资源；在ET模式下，写事件触发后，如果还需要下一次地写事件触发来驱动任务（例如发送上次剩余地数据），则我们需要继续注册一次检测可写事件。
3. LT模式和ET模式各有优缺点，无所谓孰优孰劣。LT模式时，我们可以自由决定收取多少字节或认识接收连接，但是可能会导致多次触发；使用ET模式时，我们必须每次都将数据收完或立即调用accept接收连接，其优点是触发次数少。

## 高效的readv和writev
```cpp
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
```

## 主机字节序和网络字节序
网络通信在本质上是不同的机器进行数据交换
网络字节序：big-endian

##域名解析API
gethostbyname和gethostbyaddr已经被废弃了，使用getaddrinfo代替

## 网络通信故障排查常用命令
```shell
ifconfig
ifconfig -s #精简显示
ifconfig -a #显示所有激活和未激活的网卡
ifconfig 网卡名 up
ifconfig 网卡名 down
ifconfig 网卡名 add IP地址
ifconfig 网卡名 del IP地址
ping IP地址
telnet IP地址 端口号 #检测指定了IP地址和端口号的监听服务是否存在
```
### netstat:
-a: 表示显示所有选型，不使用该选项时，默认不显示LISTEN相关选项
-t: 表示显示TCP连接
-u: 表示显示UDP连接
-n: 不显示别名，将能显示数字的全部转换为数字
-l: 仅列出监听（listen）状态的服务
-p: 显示建立相关链接的程序名
-r: 显示路由信息、路由表
-e: 显示扩展信息，例如uid
-s：按各个协议进行统计（*）
-c: 每隔一个固定的时间执行该netstat命令

### losf: list opened filedescriptor 列出已经打开的文件描述符
需要注意一下三点：
- 在默认情况下，lsof命令的输出较多，我们可以使用grep命令过滤想要查看的进行打开的fd
```shell
lsof -i | grep myapp
losf -p pid #可以查看某个进程打开的文件描述符
```
- 使用lsof只能查看当前用户有权限查看的进程fd信息
- lsof命令第一栏进程名在显示时默认显示前n个字符
```shell
#最左侧程序名最多显示15个字符
lsof +c 15
```

如果仅需显示系统的网络连接信息，则使用-i选项即可
```
lsof -Pni #对IP地址和端口号都不用别名显示
```

## nc 模拟一个服务器或客户端
### 模拟一个服务器程序
```shell
nc -v -l 127.0.0.1 6000
```
-l 在某个IP地址和端口上开启一个监听服务

### 模拟一个客户端程序
```shell
nc -v www.baidu.com 80
nc -v -p 5000 www.baidu.com 80 #指定使用哪个端口号连接服务器
```
```shell
nc -l IP地址 端口号 > 接收的文件名
nc IP地址 端口号 < 发送的文件名
```

## tcpdump
tcpdump -i 网卡名 
tcpdump -X 以ASCII和十六进制形式输出捕获的数据包内容，减去链路层的包头信息
tcpdump -XX 以ASCII和十六进制形式输出捕获的数据包内容，包含链路层的包头信息
tcpdump -n 不要将IP地址显示成别名
tcpdump -nn 不要将IP地址和端口显示成别名
tcpdump -S 以绝对值显示包的ISN号（包序列号），默认以上一包的偏移量显示
tcpdump -vv 显示详细的抓包信息
tcpdump -vvv 显示更详细的抓包信息
tcpdump -w 文件名 保存抓包的数据包到文件中
tcpdump -r 文件名 从使用-w保存的文件中读取数据包
