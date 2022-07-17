#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

bool connect_to_server(const char* server, short port)
{
  int hSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(hSocket == -1){
    printf("create socket error\n");
    return false;
  }

  struct sockaddr_in addrSrv = {0};
  struct hostent* pHostent = NULL;

  if(addrSrv.sin_addr.s_addr = inet_addr(server) == INADDR_NONE){
    pHostent = gethostbyname(server);
    if(pHostent == NULL){
      printf("gethostbyname error\n");
      return false;
    }
    addrSrv.sin_addr.s_addr = *((unsigned long*)pHostent->h_addr_list[0]);
  }

  addrSrv.sin_family = AF_INET;
  addrSrv.sin_port = htons(port);

  int ret = connect(hSocket, (struct sockaddr*)&addrSrv, sizeof(addrSrv));
  if(ret == -1){
    printf("connect error\n");
    return false;
  }
  return true;
}

int main()
{
  if(connect_to_server("www.baidu.com", 80)){
    printf("connect success\n");
  }
  else{
    printf("connect failed\n");
  }
  return 0;
}