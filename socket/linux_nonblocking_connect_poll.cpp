#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 10088
#define SEND_DATA "helloworld"

int main(int argc, char* argv[])
{
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if(clientfd == -1){
    std::cout << "create client socket error" << std::endl;
    return -1;
  }

  //set clientfd non-block
  int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
  int newSocketFlag = oldSocketFlag | O_NONBLOCK;
  if(fcntl(clientfd, F_SETFL, newSocketFlag) == -1){
    close(clientfd);
    std::cout << "set socket to nonblock error." << std::endl;
    return -1;
  }

  //connect to server
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  serveraddr.sin_port = htons(SERVER_PORT);
  for(;;){
    int ret = connect(clientfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == 0){
      std::cout << "connect to server success." << std::endl;
      close(clientfd);
      return 0;
    }
    else if(ret == -1){
      if(errno == EINTR){
        std::cout << "connecting interrupted by signal, try again." << std::endl;
        continue;
      }
      else if(errno == EINPROGRESS){
        std::cout << "connecting in progress." << std::endl;
        break;
      }
      else{
        std::cout << "connect to server error." << std::endl;
        close(clientfd);
        return -1;
      }
    }
  }
  
  pollfd event;
  event.fd = clientfd;
  event.events = POLLOUT;
  int timeout = 3000;
  if(poll(&event, 1, timeout) != 1){
    close(clientfd);
    std::cout << "[poll]connect to server error." << std::endl;
    return -1;
  }

  if(!(event.revents & POLLOUT)){
    close(clientfd);
    std::cout << "[POLLOUT]connect to server error." << std::endl;
    return -1;
  }
  
  int err;
  socklen_t len = static_cast<socklen_t>(sizeof(err));
  if(::getsockopt(clientfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0){
    close(clientfd);
    return -1;
  }

  if(err == 0){
    std::cout << "[getsockopt] connect to server success." << std::endl;
  }
  else{
    std::cout << "[getsockopt] connect to server error." << std::endl;
  }

  close(clientfd);
  return 0;
}