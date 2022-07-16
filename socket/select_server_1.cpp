#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <errno.h>

#define INVALID_FD -1

int main(int argc, char* argv[])
{
  //create a listen socket
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == INVALID_FD){
    std::cout << "create listen socket error." << std::endl;
    return -1;
  }

  //initial server address
  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10088);
  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    std::cout << "bind listen socket error." << std::endl;
    close(listenfd);
    return -1;
  }

  //start to listen
  if(listen(listenfd, SOMAXCONN) == -1){
    std::cout << "listen error." << std::endl;
    close(listenfd);
    return -1;
  }

  //save connection socket
  std::vector<int> clientfds;
  int maxfd;
  
  while(true){
    fd_set readset;
    FD_ZERO(&readset);

    //add listen socket into the checking reading events
    FD_SET(listenfd, &readset);

    maxfd = listenfd;
    //add client fd into the checking reading events
    int clientfdslength = clientfds.size();
    for(int i = 0; i < clientfdslength; ++i){
      if(clientfds[i] != INVALID_FD){
        FD_SET(clientfds[i], &readset);

        if(maxfd < clientfds[i]){
          maxfd = clientfds[i];
        }
      }
    }

    timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;
    //we only check the reading event, ignore writing and exception
    int ret = select(maxfd+1, &readset, NULL, NULL, &tm);
    if(ret == -1){
      //error, exit
      if(errno != EINTR){
        break;
      }
    }
    else if(ret == 0){
      //timeout
      continue;
    }
    else{
      //find some socket has event
      if(FD_ISSET(listenfd, &readset)){
        //new connection
        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        //accept connection from client
        int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
        if(clientfd == INVALID_FD){
          //connection error
          break;
        }
        //only accept , and do not call recv
        std::cout << "accpet a client connection, fd: " << clientfd << std::endl;
        clientfds.emplace_back(clientfd);
      }
      else{
        //assume the coming data is less than 63 char
        char recvbuf[64];
        int clientfdslength = clientfds.size();
        for(int i = 0; i < clientfdslength; ++i){
          if(clientfds[i] != INVALID_FD && FD_ISSET(clientfds[i], &readset)){
            memset(recvbuf, 0, sizeof(recvbuf));
            //not listen fd, recv data
            int length = recv(clientfds[i], recvbuf, 64, 0);
            if(length <= 0){
              //recv data error
              std::cout << "recv data error, clientfd: " << clientfds[i] << std::endl;
              close(clientfds[i]);
              clientfds[i] = INVALID_FD;
              continue;
            }
            std::cout << "clientfd: " << clientfds[i] << ", recv data: " << recvbuf << std::endl;
          }

        }
      }
    }
  }
  //close all client connection
  int clientfdslength = clientfds.size();
  for(int i = 0; i < clientfdslength; ++i){
    if(clientfds[i] != INVALID_FD){
      close(clientfds[i]);
    }
  }

  //close listen socket
  close(listenfd);
  return 0;
}