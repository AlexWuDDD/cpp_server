//verify different between LT and ET
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
#include <errno.h>
#include <iostream>

int main()
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd == -1){
    std::cout << "create listen socket error" << std::endl;
    return -1;
  }

  int on = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

  int oldSocketFlag = fcntl(listenfd, F_GETFL, 0);
  int newSocketFlag = oldSocketFlag | O_NONBLOCK;
  if(fcntl(listenfd, F_SETFL, newSocketFlag) == -1){
    close(listenfd);
    std::cout << "set listenfd to nonblock error" << std::endl;
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bindaddr.sin_port = htons(10088);

  if(bind(listenfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    close(listenfd);
    std::cout << "bind listenfd error" << std::endl;
    return -1;
  }

  if(listen(listenfd, SOMAXCONN) == -1){
    close(listenfd);
    std::cout << "listen listenfd error" << std::endl;
    return -1;
  }

  int epollfd = epoll_create(1);
  if(epollfd == -1){
    close(listenfd);
    std::cout << "create epollfd error" << std::endl;
    return -1;
  }

  epoll_event listen_fd_event;
  listen_fd_event.data.fd = listenfd;
  listen_fd_event.events = EPOLLIN;

  listen_fd_event.events |= EPOLLET;

  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &listen_fd_event) == -1){
    close(listenfd);
    close(epollfd);
    std::cout << "epoll_ctl listenfd error" << std::endl;
    return -1;
  }

  int n;
  while(true){
    epoll_event epoll_events[1024];
    n = epoll_wait(epollfd, epoll_events, 1024, 1000);
    if(n < 0){
      if(errno == EINTR){
        continue;
      }
      std::cout << "epoll_wait error" << std::endl;
      break;
    }
    else if(n == 0){
      std::cout << "epoll_wait timeout" << std::endl;
      continue;
    }

    for(size_t i= 0; i < n; ++i){
      if(epoll_events[i].events & EPOLLIN){
        if(epoll_events[i].data.fd == listenfd){
          struct sockaddr_in clientaddr;
          socklen_t clientaddrlen = sizeof(clientaddr);
          int clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddrlen);
          if(clientfd != -1){
            int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
            int newSocketFlag = oldSocketFlag | O_NONBLOCK;
            if(fcntl(clientfd, F_SETFL, newSocketFlag) == -1){
              close(clientfd);
              std::cout << "set clientfd to nonblock error" << std::endl;
            }
            else{
              epoll_event client_fd_event;
              client_fd_event.data.fd = clientfd;
              client_fd_event.events = EPOLLIN;
              client_fd_event.events |= EPOLLET;
              if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &client_fd_event) != -1){
                std::cout << "new client accept, clientfd: " << clientfd << std::endl;
              }
              else{
                std::cout <<"add client fd to epollfd error" << std::endl;
                close(clientfd);
              }
            }
          }
        }
        else{
          std::cout << "client fd: " << epoll_events[i].data.fd << " read data" << std::endl;
          char ch;
          int m = recv(epoll_events[i].data.fd, &ch, 1, 0);
          if(m == 0){
            //peer close, remove fd from epollfd
            if(epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) != -1){
              std::cout << "client disconnected, clientfd: " << epoll_events[i].data.fd << std::endl;
            }
            close(epoll_events[i].data.fd);
          }
          else if(m < 0){
            if(errno != EWOULDBLOCK && errno != EINTR){
              if(epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) != -1){
                std::cout << "client disconnected, clientfd: " << epoll_events[i].data.fd << std::endl;
              }
              close(epoll_events[i].data.fd);
            }
          }
          else{
            std::cout << "client fd: " << epoll_events[i].data.fd << " read data: " << ch << std::endl;
          }
        }
      }
      else if(epoll_events[i].events & EPOLLERR){
        //TODO: do nothing now
      }
    }
  }
  close(listenfd);
  close(epollfd);
  return 0;
}