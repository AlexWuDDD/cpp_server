#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <errno.h>

#define INVALID_FD -1

int main(int argc, char* argv[])
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == INVALID_FD){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }
  
  int oldSocketFlag = fcntl(listenfd, F_GETFL, 0);
  int newSocketFlag = oldSocketFlag | O_NONBLOCK;
  if(fcntl(listenfd, F_SETFL, newSocketFlag) == -1){
    close(listenfd);
    std::cout << "set listen socket to nonblock error." << std::endl;
    return -1;
  }
  
  int on = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
  
  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10088);
  
  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    close(listenfd);
    std::cout << "bind listen socket error." << std::endl;
    return -1;
  }
  
  if(listen(listenfd, SOMAXCONN) == -1){
    close(listenfd);
    std::cout << "listen error." << std::endl;
    return -1;
  }
  
  std::vector<pollfd> pollfds;
  pollfd listen_fd_info;
  listen_fd_info.fd = listenfd;
  listen_fd_info.events = POLLIN;
  listen_fd_info.revents = 0;
  pollfds.push_back(listen_fd_info);

  bool exist_invalid_fd;
  int n;
  while(true){
    exist_invalid_fd = false;
    n = poll(&pollfds[0], pollfds.size(), 1000);
    if(n<0){
      if(errno == EINTR){
        continue;
      }
      //something wrong
      break;
    }
    else if(n == 0){
      //timeout
      continue;
    }

    for(size_t i = 0; i < pollfds.size(); ++i){
      //read event
      if(pollfds[i].revents & POLLIN){
        if(pollfds[i].fd == listenfd){
          struct sockaddr_in clientaddr;
          socklen_t clientaddrlen = sizeof(clientaddr);
          int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
          if(clientfd != -1){
            int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
            int newSocketFlag = oldSocketFlag | O_NONBLOCK;
            if(fcntl(clientfd, F_SETFL, newSocketFlag) == -1){
              close(clientfd);
              std::cout << "set client socket to nonblock error." << std::endl;
            }
            else{
              pollfd client_fd_info;
              client_fd_info.fd = clientfd;
              client_fd_info.events = POLLIN;
              client_fd_info.revents = 0;
              pollfds.push_back(client_fd_info);
              std::cout << "new client accpeted, clientfd: " << clientfd << std::endl;
            }
          }
        }
        else{
          //normal client socket, recv data
          char buf[64] = {0};
          int m = recv(pollfds[i].fd, buf, sizeof(buf), 0);
          if(m <= 0){
            if(errno != EINTR && errno != EWOULDBLOCK){
              //error
              for(std::vector<pollfd>::iterator iter = pollfds.begin(); iter != pollfds.end(); ++iter){
                if(iter->fd == pollfds[i].fd){
                  std::cout << "client disconnected, clientfd: " << iter->fd << std::endl;
                  close(iter->fd);
                  iter->fd = INVALID_FD;
                  exist_invalid_fd = true;
                  break;
                }
              }
            }
          }
          else{
            std::cout << "recv data from client, clientfd: " << pollfds[i].fd << ", data: " << buf << std::endl;
          }
        }
      }
      else if(pollfds[i].revents & POLLERR){
        //TODO: do nothing now
      }
    }

    if(exist_invalid_fd){
      for(std::vector<pollfd>::iterator iter = pollfds.begin(); iter != pollfds.end(); ++iter){
        if(iter->fd == INVALID_FD){
          pollfds.erase(iter);
          break;
        }
      }
    }
  }

  for(std::vector<pollfd>::iterator iter = pollfds.begin(); iter != pollfds.end(); ++iter){
    if(iter->fd != INVALID_FD){
      close(iter->fd);
    }
  }

  return 0;
}