## HTTP 格式
HTTP是建立在TCP之上的应用层协议，其格式如下
```plain
GET或POST请求的URL路径（一般是去掉域名的路径）HTTTP协议版本号\r\n
字段1名： 字段1值\r\n
字段2名： 字段2值\r\n
...
字段n名： 字段n值\r\n
\r\n
HTTP包体内容
```
**我们在用程序解析HTTP格式的数据时可以通过\r\n\r\n界定包头的结束位置和包体的起始位置**

## HTTP chunk编码
HTTP在传输过程中如果包体过大，比如使用HTTP上传一个大文件，或者传输动态产生的内容给对端时，
传输方无法预先知道传输的内容有多大，这时就可以使用HTTP chunk编码技术。
### 原理
将整个HTTP包体分成多个小块，每一块都有自己的字段来说明自身的长度，对端收到这些块后，去除说明的部分，
将多个小块合并在一起得到完整的包体内容。
传输方在HTTP包头中设置了**Transfer-Encoding: chunked**来告诉对端这个数据时分块传输的（代替Content-Length字段）
```plain
格式如下：
[chunkSize][\r\n][chunkData][\r\n][chunkSize][\r\n][chunkData][\r\n][chunkSize=0][\r\n][\r\n]
```
### 对端对chunk格式的解压缩
首先，对端要在收到的HTTP头部找到Transfer-Encoding字段，并且其值是chunked
1. 找到HTTP包体开始的地方（HTTP头部\r\n\r\n）的下一个位置)和接下来一个\r\n中间的部分，这是第一个chunkSize的内容。
2. 假设第一个chunkSize的长度是n字节，则对照ASCII码表将这n字节依次转换成数字，然后将这些数字拼凑起来当成十六进制，再转换成十进制，这就是接下来chunkData的长度。
3. 跳过\r\n，获取下一个数据块的chunkSize和chunkData，直到chunkSize=0的数据块
4. 将各个数据块的chunkData拼接起来，就是完整的HTTP包体内容

## HTTP服务端实现
### 收到数据后如下逻辑进行处理
```cpp
//GET 请求
void HttpSession:OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receiveTime)
{
  string inbuf;
  //先把所有数据取出来
  inbuf.append(pBuffer->peek(), pBuffer->readableBytes());
  //因为一个HTTP包头的数据至少有\r\n\r\n，所以大于4个字符
  //若小于或等于4个字符，则说明数据未完成，退出，等待网络底层接着收取
  if(inbuf.length() <= 4){
    return;
  }

  //检查是否以\r\n\r\n结束，如果不是，则说明包头不完整，退出
  string end = inbuf.substr(inbuf.length() - 4);
  if(end != "\r\n\r\n"){
    if(inbuf.length() > MAX_URL_LENGTH){
      //超过2048个字符，且不含有\r\n\r\n，我们认为是非法请求
      conn->forceClose();
    }

    //检查是否以\r\n\r\n结束，如果不是，且长度不超过2048个字符
    //则说明包头不完整，退出
    return;
  }

  //以\r\n分割每一行
  std::vector<string> lines;
  SpringUtil:Split(inbuf, lines, "\r\n");
  if(lines.size() < 1 || lines[0].empty()){
    conn->forceClose();
    return;
  }
  
  std::vector<string> chunk;
  StringUtil:Split(lines[0], chunk, " ");
  //在chunk中至少有3个字符串，GET、URL、HTTP版本号
  if(chunk.size() < 3){
    conn->forceClose();
    return;
  }

  LOG_INFO << "url: " << chunk[1] << " from " << conn->peerAddress().toIpPort();
  //通过？分割成前后两端，前面是URL，后面是参数
  std::vector<string> part;
  StringUtil:Split(chunk[1], part, "?");
  if(part.size() < 2){
    conn->forceClose();
    return;
  }

  string url = part[0];
  string param = part[1].substr(2);
  if(!Process(conn, url, param)){
    LOG_ERROR << "Process failed";
  }

  //短连接，处理完后关闭
  conn->forceClose();
}
```
### 应答协议的格式如下
```plain
GET或POST 响应码 HTTP协议版本号\r\n
字段1名：字段1值\r\n
字段2名：字段2值\r\n
...
字段n名：字段n值\r\n
\r\n
HTTP包体内容
```

