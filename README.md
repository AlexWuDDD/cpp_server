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
