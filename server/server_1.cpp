#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>


int main()
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == -1){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(3030);
  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    std::cout << "bind listend socket error" << std::endl;
    return -1;
  }

  if(listen(listenfd, SOMAXCONN) == -1){
    std::cout << "listen error" << std::endl;
    return -1;
  }

  while(true){
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
    if(clientfd != -1){
      char recvBuf[32] = {0};
      int ret = recv(clientfd, recvBuf, sizeof(recvBuf), 0);
      if(ret > 0){
        std::cout << "recv data from client, data: " << recvBuf << std::endl;
        ret = send(clientfd, recvBuf, sizeof(recvBuf), 0);
        if(ret != strlen(recvBuf)){
          std::cout << "send data to client error" << std::endl;
        }

        std::cout << "send data to client successfully, data: " << recvBuf << std::endl;

      }
      else{
        std::cout << "recv data error." << std::endl;
      }

      close(clientfd);
    }
  }

  close(listenfd);
  return 0;
}
