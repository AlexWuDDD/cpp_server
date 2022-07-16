//verify the send behavior in blocking mode
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

int main(int argc, char* argv[])
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == -1){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10088);
  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    std::cout << "bind listen socket error." << std::endl;
    close(listenfd);
    return -1;
  }

  if(listen(listenfd, SOMAXCONN) == -1){
    std::cout << "listen socket error." << std::endl;
    close(listenfd);
    return -1;
  }

  while(true){
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen = sizeof(clientaddr);
    int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
    if(connfd != -1){
      //only accept connection, and not recv any data
      std::cout << "accept a client connection." << std::endl;
    }
  }
  close(listenfd);
  return 0;
}