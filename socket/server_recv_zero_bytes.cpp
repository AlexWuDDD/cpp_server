#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>

int main(int argc, char* argv[])
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10088);

  if(bind(listenfd, (struct sockaddr*)(&bindaddr), sizeof(bindaddr)) == -1){
    std::cout <<"bind listen socket error." << std::endl;
    close(listenfd);
    return -1;
  }

  if(listen(listenfd, SOMAXCONN) == -1){
    std::cout << "listen error." << std::endl;
    close(listenfd);
    return -1;
  }

  int clientfd;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);
  clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
  if(clientfd != -1){
    while(true){
      char recvBuf[32] = {0};
      int ret = recv(clientfd, recvBuf, 32, 0);
      if(ret > 0){
        std::cout << "recv data from client, data: " << recvBuf << std::endl;
      }
      else if(ret == 0){
        std::cout << "recv 0 byte data." << std::endl;
        continue;
      }
      else{
        std::cout << "recv data error."<< std::endl;
        break;
      }
    }
  }
  close(clientfd);
  close(listenfd);
  return 0;
}