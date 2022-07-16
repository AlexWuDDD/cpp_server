//verify the recv behavior in blocking mode
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

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

  int oldSocketFlags = fcntl(clientfd, F_GETFL, 0);
  int newSocketFlags = oldSocketFlags | O_NONBLOCK;
  if(fcntl(clientfd, F_SETFL, newSocketFlags) == -1){
    std::cout << "set client socket to nonblocking error." << std::endl;
    close(clientfd);
    return -1;
  }

  while(true){
    char recvbuf[32] = {0};
    int ret = recv(clientfd, recvbuf, 32, 0);
    if(ret > 0){
      std::cout << "recv data success. data = " << recvbuf << std::endl;
    }
    else if(ret == 0){
      std::cout << "peer close the socket" << std::endl;
      break;
    }
    else{
      if(errno == EWOULDBLOCK){
        std::cout << "There is no data available" << std::endl;
      }
      else if(errno ==EINTR){
        std::cout << "recv is interrupted" << std::endl;
      }
      else{
        std::cout << "recv data error. errno = " << errno << std::endl;
        break;
      }
    }
  }

  close(clientfd);
  return 0;
}