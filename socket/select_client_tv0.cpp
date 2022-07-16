#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <errno.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT    10088

int main(int argc, char* argv[])
{
  //create a client socket
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if(clientfd == -1){
    std::cout << "create client socket error." << std::endl;
    return -1;
  }

  //connect to server
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  serveraddr.sin_port = htons(SERVER_PORT);

  if(connect(clientfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){
    std::cout << "connect to server error." << std::endl;
    close(clientfd);
    return -1;
  }

  int ret;
  while(true){
    fd_set readset;
    FD_ZERO(&readset);
    //ADD client socket into the checking reading events
    FD_SET(clientfd, &readset);
    timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = 0;

    ret = select(clientfd + 1, &readset, NULL, NULL, &tm);
    std::cout << "tm.tv_sec = " << tm.tv_sec << ", tm.tv_user: " << tm.tv_usec <<std::endl;
    if(ret == -1){
      if(errno != EINTR){
        break;
      }
    }
    else if(ret == 0){
      //timeout
      std::cout << "no event in specific time interval." << std::endl;
      continue;
    }
    else{
      if(FD_ISSET(clientfd, &readset)){
        char recvbuf[32];
        memset(recvbuf, 0, sizeof(recvbuf));
        int n = recv(clientfd, recvbuf, sizeof(recvbuf), 0);
        if(n < 0){
          if(errno != EINTR){
            break;
          }
        }
        else if(n ==0){
          //server close the connection
          std::cout << "server close the connection." << std::endl;
          break;
        }
        else{
          std::cout << "recv data: " << recvbuf << std::endl;
        }
      }
      else{
        std::cout << "other socket event" << std::endl;
      }
    }
  }

  close(clientfd);
}