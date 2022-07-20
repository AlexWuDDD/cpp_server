## webSocket 
用于解决HTTP通讯的：
- 无状态
- 短连接（通常是）
- 服务端无法主动向客户端推送数据等问题

通信基础基于TCP，使用websocket协议的通信双方在进行TCP三次握手之后，还要额外地进行一次握手，这次参与握手的报文格式时基于HTTP改造的。

这个报文的格式大致如下：
```plain
GET /realtime HTTP/1.1 /r/n
Host: 127.0.0.1:9989\r\n
Connection: Upgrade\r\n
Pragma: no-cache\r\n
Cache-Control: no-cache\r\n
User-Ageny: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n
Upgrade: websocket\r\n
Origin: http://xyz.com\r\n
Sec-WebSocket-Version: 13\r\n
Accept-Encoding: gzip, deflate, br\r\n
Accept-Language: zh-CN, zh;q=0.9,en;q=0.8\r\n
Sec-WebSocket-Key: IqcAscsvwvwevdved==\r\n
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n
```
对这个格式有如下要求：
1. 握手必须时一个有效的HTTP请求
2. 请求的方法必须时GET, 且HTTP版本必须时1.1
3. 请求必须包含Host字段信息
4. 请求必须包含Upgrade字段信息，值必须是websocket
5. 请求必须包含Connection字段信息，值必须是Upgrade
6. 请求必须包含Sec-WebSocket-Key字段，该字段值是客户端的标识编码成base64格式后的内容
7. 请求必须包含Sec-WebSocket-Version字段信息，值必须是13
8. 请求必须包含Origin字段
9. 请求可能包含Sec-WebSocket-Protocol字段来规定子协议
10. 请求可能包含Sec-WebSocket-Extensions字段来规定协议的扩展
11. 请求可能包含其他字段如cookie等

应答格式的报文格式大致如下：
```plain
HTTP/1.1 101 Switching Protocols\r\n
Upgrade: websocket\r\n
Connection: Upgrade\r\n
Sec-WebSocket-Accept: 5csscscscsdvwevre34=\r\n
```

对于Sec-WebSocket-Accept字段，其值是根据对端传过来的Sec-WebSocket-Key的值经过一定的算法计算出来的，这样应答的双方才能匹配。
```cpp
//算法公式如下：
mask = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
accpet = base64(sha1(Sec-WebSocket-Key + mask));
```

websocket 压缩和解压缩算法即gzip压缩算法
压缩和解压缩的函数为zlib库的deflate(压缩)和inflate(解压缩)
>注意，在使用zlib的deflate函数进行压缩时，压缩完毕后要将压缩后的字节流末尾多余的4字节删掉


### Websocket 协议的基本结构
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+




