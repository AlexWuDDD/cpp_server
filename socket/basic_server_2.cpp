#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>

int main(int argc, char* argv[])
{
  //1. create a listening socket
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == -1){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  //2. initail server address
  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10086);
  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    std::cout << "bind listen socket error" << std::endl;
    return -1;
  }

  //3. start to listen
  if(listen(listenfd, SOMAXCONN) == -1){
    std::cout << "listen error" << std::endl;
    return -1;
  }

  //container for logging all the clienct connection
  std::vector<int> clientfds;
  while(true){
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    //4. accept client connection
    int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if(clientfd != -1){
      char recvBuf[32] = {0};
      //5. receive data from client
      int ret = recv(clientfd, recvBuf, 32, 0);
      if(ret > 0){
        std::cout << "recv data from client, data: " << recvBuf << std::endl;
        //6. send the received datw without change back to the client
        ret = send(clientfd, recvBuf, strlen(recvBuf), 0);
        if(ret != strlen(recvBuf)){
          std::cout << "send data error." << std::endl;
        }
        else{
          std::cout << "send data to client successfully, data: " << recvBuf << std::endl;
        }
      }
      else{
        std::cout << "recv data error." << std::endl;
      }

      // close(clientfd);
      clientfds.emplace_back(clientfd);
    }
  }

  //7. close listen socket
  close(listenfd);

  return 0;
}