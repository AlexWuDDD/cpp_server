# 服务器开发中的常用模块设计
## 心跳检测一般有两个作用：保存和检测死链

### TCP keepalive选项
```cpp
//on为1时表示打开keepalive选项，为0时表示关闭，0是默认值
int on = 1;
setsockopt(fd, SOL_SOCKET, SOL_KEEPALIVE, &on, sizeof(on));
```
默认心跳检测的数据包的时间间隔是7200秒。
可以通过以下方式修改
```cpp
//发送keepalive报文的时间间隔
int val = 7200;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,&val, sizeof(val));

//两次重试报文的时间间隔
int interval = 75;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));

int cnt = 9;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt)); //重试9次
```

### 应用层的心跳包机制设计
keepalive选项不能与应用层很好地交互，因此在实际的服务开发中，**还是建议在应用层设计自己的心跳包机制**
对于保活的心跳包，最佳做法是记录最近一次收发数据包的时间，在每次收数据和发数据时都更新这个时间。
而心跳检测计时器在每次检测时都将这个时间与当前系统做比较，如果时间间隔大于允许的最大时间间隔（在实际开发中，将其设置成15~45秒不等），则发送一次心跳包。
总而言之，就是在与对端之间数据数据来往达到一定时间间隔时，才发送一次心跳包。

我们一般在调试模式下通过配置开关变量或者条件编译选项关闭心跳包检测逻辑。
##### 通过开关量控制心跳包检测逻辑开启
```cpp
if(config.heartbeatCheckEnabled)
{
  EnableHeartbeatCheck();
}
```
##### 通过条件编译控制心跳包检测逻辑开启
```cpp
#ifndef _DEBUG
 EnableHeartbeatCheck();
#endif
```









