#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#include <iostream>
#include <string.h>
#include <vector>

int main()
{
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1)
  {
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  int on = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

  int oldSocketFlag = fcntl(listen_fd, F_GETFL, 0);
  int newSocketFlag = oldSocketFlag | O_NONBLOCK;
  if(fcntl(listen_fd, F_SETFL, newSocketFlag) == -1)
  {
    std::cout << "set listenfd socket to nonblock error" << std::endl;
    close(listen_fd);
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(9999);

  if(bind(listen_fd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1)
  {
    std::cout << "bind listen socket error" << std::endl;
    close(listen_fd);
    return -1;
  }

  if(listen(listen_fd, SOMAXCONN) == -1)
  {
    std::cout << "listen listen socket error" << std::endl;
    close(listen_fd);
    return -1;
  }

  int epoll_fd = epoll_create(1);
  if(epoll_fd == -1)
  {
    std::cout << "create epollfd error" << std::endl;
    close(listen_fd);
    return -1;
  }

  struct epoll_event listen_fd_event;
  listen_fd_event.data.fd = listen_fd;
  listen_fd_event.events = EPOLLIN;

  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_fd_event) == -1){
    std::cout << "epoll_ctl error" << std::endl;
    close(listen_fd);
    close(epoll_fd);
    return -1;
  }
  
  int n;
  while(true){
    epoll_event epoll_events[1024];
    n = epoll_wait(epoll_fd, epoll_events, 1024, 1000);
    if(n < 0){
      if(errno == EINTR){
        continue;
      }

      break;
    }
    else if(n == 0){
      //timeout
      continue;
    }

    for(size_t i = 0; i < n; ++i){
      if(epoll_events[i].events & EPOLLIN){
        if(epoll_events[i].data.fd == listen_fd){
          struct sockaddr_in clientaddr;
          socklen_t clientaddrlen = sizeof(clientaddr);
          int clientfd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
          if(clientfd != -1){
            int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
            int newSocketFlag = oldSocketFlag | O_NONBLOCK;
            if(fcntl(clientfd, F_SETFL, newSocketFlag) == -1){
              std::cout << "set clientfd socket to nonblock error" << std::endl;
              close(clientfd);
            }
            else{
              struct epoll_event client_fd_event;
              client_fd_event.data.fd = clientfd;
              client_fd_event.events = EPOLLIN | EPOLLONESHOT;
              if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &client_fd_event) == -1){
                std::cout << "epoll_ctl error" << std::endl;
                close(clientfd);
              }
              else{
                std::cout << "accept a new client, clientfd: " << clientfd << std::endl;
              }
            }
          }
        }
        else{
          std::cout << "client fd: " << epoll_events[i].data.fd << " read data" << std::endl;
          char ch;
          int m = recv(epoll_events[i].data.fd, &ch, 1, 0);
          if(m == 0){
            //peer close
            if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) == -1){
              std::cout << "epoll_ctl error" << std::endl;
            }
            else{
              std::cout << "client fd: " << epoll_events[i].data.fd << " close" << std::endl;
              close(epoll_events[i].data.fd);
            }
          }
          else if(m < 0){
            if(errno != EWOULDBLOCK && errno != EINTR){
              //error
              if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) == -1){
                std::cout << "epoll_ctl error" << std::endl;
              }
              else{
                std::cout << "client fd: " << epoll_events[i].data.fd << " close" << std::endl;
                close(epoll_events[i].data.fd);
              }
            }
          }
          else{
            //read data
            std::cout << "recv from client: " << epoll_events[i].data.fd << ", data: " << ch << std::endl;
          }
        }
      }
      else if(epoll_events[i].events & EPOLLERR){
        //TODO do nothing now
      }
    }
  }

  close(listen_fd);
  close(epoll_fd);
  return 0;
}