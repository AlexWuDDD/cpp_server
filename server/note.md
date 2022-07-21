## 服务器程序的高性能、高并发
从技术角度讲
### 高性能：程序能流畅、低延迟地应答客户端的各类请求
### 高并发：程序可以在同一时间支持较多的客户端连接和数据请求

**网络通信组件是服务器程序的基础组件，其设计得好坏直接影响服务器对外服务的能力**

## 高效网络通信框架的设计原则
### 1.尽量少等待
#### 一个好的网络通信架构至少要解决以下7个问题：
1. 如何检测有新的客户端连接到来？
2. 如何接受客户端的连接请求？
3. 如何检测客户端是否有数据发送过来？
4. 如何收取客户单发送的数据？
5. 如何检测异常的客户端连接？检测到之后，如何处理？
6. 如何向客户端发送数据？
7. 如何在客户端发送数据后关闭连接？
>使用IO服用技术即可。

### 2.尽量少做无用功
对于服务器网络通信组件来说，要想高效，就应该尽量避免花时间主动查询一些socket是否有网络事件，而是等到这些socket有网络事件时让系统主动通知我们，我们再去处理。
> 使用I/O复用API，如果某个socket失效，就应该及时从I/O复用API上移除该socket，否则可能造成死循环或者浪费CPU检测周期的问题。

### 3.检测网络事件的高效做法

需要说明的是，TCP连接是状态机，I/O复用函数一般无法检测出两个端点之间路由错误导致的链路问题，所以对于这种情形，我们需要定时器结合心跳包来检测。

> 在epoll模型水平触发模式下，如果有数据要发送，则将剩余的数据先缓存起来（需要一个缓冲区来存放剩余的数据，即“发送缓冲区”），再为该socket注册写事件标志，等下次写事件触发时再发送剩余的数据；如果剩下的数据还是不能完全发送完，则继续等待下一次写事件触发通知，在下一次写事件触发通知后，继续发送数据。如此反复，直到所有数据都发送出去为止。一旦所有数据都发送出去了，就及时为socket移除检测写事件标志，避免再次触发无用的写事件通知。


## one thread one loop

```cpp
while(!m_bQuitFlag){
  epoll_or_select_fun();
  handle_io_events();
  handle_other_things();
}
```

>我们想要达到的效果是：如果没有网络I/O事件和其他任务要处理，那么这些工作线程最好直接挂起而不是空转；如果有其他任务要处理，那么这些工作要能立刻处理这些任务，而不是在poll、select、epoll_wait等函数处挂起指定时间后才处理其他任务。

解决方案是：我们仍然给poll,select, epoll_wait等函数设置一定的超时时间（大于0），但对于handle_other_things 函数的执行，会采取一种特殊的唤醒策略。以Linux的epoll_wait函数为例，不管在epollfd上有没有socket fd，我们都会为epollfd挂载一个特殊的fd，这个fd被称为wakeup fd（唤醒fd）。当我们有其他任务需要立即处理时（也就需要handle_other_thing()立刻执行时），向这个唤醒fd上随便写入1字节的内容，这个唤醒fd就立即变成可读的了，epoll_wait函数会立即被唤醒并返回，接下来就可以马上执行hanlde_other_thing函数了，这样其他任务就可以得到立即处理；反之，在没有其他任务也没有网络I/O事件时，epoll_or_select_func函数就挂在那里什么也不做。

#### 唤醒机制的实现 in Linux
##### 1.使用管道fd(pipe)
创建一个管道，将管道的一段（管道fd中的一个）绑定到epollfd上，需要唤醒时，向管道的另一端（管道fd中的另一个）写入1字节，工作线程会被立即唤醒。创建管道的相关API函数。

```cpp
#include <unistd.h>
#include <fcntl.h>

int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
```

##### 2.使用Linux2.6新增的eventfd
eventfd的使用方法与管道fd一样，将调用eventfd函数返回的eventfd绑定到epollfd上，需要唤醒时，向这个eventfd上写入一字节，I/O复用函数会被立即唤醒。创建eventfd函数签名如下：
```cpp
#include <sys/eventfd.h>

int eventfd(unsigned int interval, int flags);
```

##### 3.socketpair
socketpair是一对相互连接的socket，相当于服务端和客户端的两个端点，每一端都可以读写数据，向其中一端写入数据后，就可以从另一端读取数据了。创建socketpair的函数签名如下：
```cpp
#include <sys/types.h>
#include <sys/socket.h>

int socketpair<int domain, int type, int protocol, int sv[2]); //创建socketpair时，第1个参数domain必须设置为AF_UNIX。
```

### 带定时器的程序结构
```cpp
while(!m_bQuitFlag){
  check_and_handle_timers(); 
  epoll_or_select_fun();
  handle_io_events();
  handle_other_things();
}
```
> 在epoll_or_select_fun()中使用I/O复用函数的超时时间要尽量不大于check_and_handle_timers()中所有定时器的最小时间间隔，以免定时器的逻辑处理延迟较大。

**I/O线程（loop所在的线程）本身不能处理耗时的操作。**

## 收发数据的正确做法
- 使用select、poll或epoll LT模式时，可以直接为待检测fd注册监听读事件标志。
- 使用select、poll或epoll LT模式时，不要直接为待检测fd注册监听可写事件标志，应该先尝试发送数据，若TCP窗口太小发布出去，则再为待检测fd注册监听可写事件标志，一旦数据发送完，就应该立即移除监听写事件标志。
- 使用epoll ET模式时，如果需要发送数据，则每次都要为fd注册监听写事件标志

## 不要多个线程同时利用一个socket收（发）数据

## 发送、接收缓冲区的设计要点
一般都建议将其设计成一个内存连续的存储容器。
通常情况下，至少需要提供两类接口，即存数据的接口和取数据的接口、FIFO
容量设置成多大，应该像std::string、vector一样，设计出一个容量可以动态增加的结构，按需分配，容量不够时可以扩展容量。
需要收发数据的缓冲区，提供写入和读取各种数据类型的接口。

## 网络库的分层设计

**对于计算机科学领域中的任何问题，都可以通过增加一个间接的中间层来解决**

### 网络库设计中的各个层

- Session层 (业务层): 用于记录各种业务状态数据和处理各种业务逻辑。在业务逻辑处理后，如果需要进行网络通信，则依赖Connection层进行数据收发。
- Connection层 (技术层): 每一路客户端连接都对应一个Connection对象，该层一般用于记录连接的各种状态信息，掌管Channel的生命周期
- Channel层(技术层)：实际进行数据收发的地方, 直接掌管socket的生命周期
- Socket层(技术层)：对常用的socket函数进行封装，屏蔽不同操作系统socket操作的差异性


## 后端服务中的定时器设计
#### 一个最简单的定时器功能可以按如下思路实现：
```cpp
void* threadFunc(void& arg){
  while(m_bRunning){
    //休眠3秒
    sleep(3000);
    //检测所有会话的心跳
    checkSessionHeartbeat();
  
  }

}
```

std::priority_queue作为定时器的实现

> 定时器的实现原理和逻辑并不复杂，关键点时如何为**定时器对象**集合设计出高效的数据结构，使每次从定时器集合中增加、删除、修改和遍历定时器对象时都更高效。另外，为了进一步提高定时器逻辑的执行效率，在某些场景下，我们可以利用上次缓存的系统时间来避免再一次调用获取时间的系统API的开销。










