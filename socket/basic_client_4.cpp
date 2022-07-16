#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 10086
#define SEND_DATA "helloalex"

int main(int argc, char* argv[])
{
  //1. create a socket
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if(clientfd == -1){
    std::cout << "create client socket error" << std::endl;
    return -1;
  }

  struct sockaddr_in bindaddr;
  bindaddr.sin_family = AF_INET;
  bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //bind socket to 0
  bindaddr.sin_port = htons(20000);
  if(bind(clientfd, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) == -1){
    std::cout << "bind socket error." << std::endl;
    return -1;
  }

  //2. connect to the server
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  serveraddr.sin_port = htons(SERVER_PORT);
  if(connect(clientfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){
    std::cout << "connect socket error." << std::endl;
    return -1;
  }

  //3. send data to server
  int ret = send(clientfd, SEND_DATA, strlen(SEND_DATA), 0);
  if(ret != strlen(SEND_DATA)){
    std::cout << "send data error." << std::endl;
    return -1; 
  }

  std::cout << "send data successfully, data: " << SEND_DATA << std::endl;

  //4. recv data from server
  char recvBuf[32] = {0};
  ret = recv(clientfd, recvBuf, 32, 0);
  if(ret > 0){
    std::cout << "recv data successfully, data: " << recvBuf << std::endl;
  }
  else{
    std::cout << "recv data error, data: " << recvBuf << std::endl;
  }

  //5. close socket
  // close(clientfd);
  while(true){
    sleep(3);
  }

  return 0;
}