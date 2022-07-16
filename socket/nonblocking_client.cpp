//verify the send behavior in blocking mode
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 10088
#define SEND_DATA "hello world"

int main(int argc, char* argv[])
{
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if(clientfd == -1){
    std::cout << "create client socket error" << std::endl;
    return -1;
  }

  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  serveraddr.sin_port = htons(SERVER_PORT);
  if(connect(clientfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){
    std::cout << "connect server error." << std::endl;
    close(clientfd);
    return -1;
  }

  //after connection, we change clientfd to nonblocking mode
  //we cannot set when create client socket, because it will effect the behavior of connect()
  int oldSocketFlags = fcntl(clientfd, F_GETFL, 0);
  int newSocketFlags = oldSocketFlags | O_NONBLOCK;
  if(fcntl(clientfd, F_SETFL, newSocketFlags) == -1){
    std::cout << "set client socket to nonblocking error." << std::endl;
    close(clientfd);
    return -1;
  }

  //keeping send data to server until error and break loop
  int count = 0;
  while(true){
    int ret = send(clientfd, SEND_DATA, strlen(SEND_DATA), 0);
    if(ret == -1){
      //nonblocking mode, send cannot send data due TCP window is too small, and errno is EWOULDBLOCK
      if(errno == EWOULDBLOCK){
        std::cout << "send data error as TCP Window size is too small." << std::endl;
        continue;
      }
      else if(errno == EINTR){
        std::cout << "send data interrupted by signal." << std::endl;
        continue;
      }
      else{
        std::cout << "send data error." << std::endl;
        break;
      }
    }
    else if(ret == 0){
      std::cout << "send data error due to peer close." << std::endl;
      close(clientfd);
      break;
    }
    else{
      ++count;
      std::cout << "send data success. count = " << count <<std::endl;
    }
  }
  close(clientfd);
  return 0;
}