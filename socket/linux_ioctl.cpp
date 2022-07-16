//how many bytes in recv buffer of socket

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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
    if(n < 0){
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

    int size = pollfds.size();
    for(size_t i = 0; i < size; ++i){
      //read event
      if(pollfds[i].revents & POLLIN){
        if(pollfds[i].fd == listenfd){
          //listen socket, accept new connection
          struct sockaddr_in clientaddr;
          socklen_t clientaddrlen = sizeof(clientaddr);
          int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
          if(clientfd != INVALID_FD){
            //set client socket to nonblock
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
              std::cout << "new client accpted, clientfd: " << clientfd << std::endl;
            }
          }
        }
        else{
          //get recv buffer bytes num
          ulong bytesToRecv = 0; //must init to 0
          if(ioctl(pollfds[i].fd, FIONREAD, &bytesToRecv) == 0){
            std::cout << "bytesToRecv: " << bytesToRecv << std::endl;
          }

          /*
          !!!!!!!!!!!!!!!!!!!!!!!!!!!!
          we cannot define recv buf size based on bytesToRecv
          beacuse before we call the recv(), real bytesToRecv will be increased
          !!!!!!!!!!!!!!!!!!!!!!!!!!!!
          */

          //normal clientfd socket, recv data
          char buf[64] = {0};
          int m = recv(pollfds[i].fd, buf, sizeof(buf), 0);
          if(m <= 0){
            if(errno != EINTR && errno != EWOULDBLOCK){
              std::cout << "client disconnected, clientfd: " << pollfds[i].fd <<std::endl;
              close(pollfds[i].fd);
              pollfds[i].fd = INVALID_FD;
              exist_invalid_fd = true;
            }
          }
          else{
            std::cout << "recv from client: " << buf << ", clientfd: " << pollfds[i].fd << std::endl;
          }
        }
      }
      else if(pollfds[i].revents * POLLERR){
        //TODO: DO NOTHING NOW
      }
    }

    if(exist_invalid_fd){
      //remove invalid fd
      for(std::vector<pollfd>::iterator it = pollfds.begin(); it != pollfds.end();){
        if(it->fd == INVALID_FD){
          it = pollfds.erase(it);
        }
        else{
          ++it;
        }
      }
    }
  }

  for(std::vector<pollfd>::iterator it = pollfds.begin(); it != pollfds.end(); ++it){
    close(it->fd);
  }
  return 0;
}