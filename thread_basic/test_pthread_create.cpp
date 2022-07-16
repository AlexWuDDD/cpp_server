#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

void* threadfunc(void *arg){
  while(1){
    //sleep 1s
    sleep(1);
    printf("I am a New Thread\n");
  }

  return NULL;
}

int main(){
  pthread_t threadid;
  
  int rc = pthread_create(&threadid, NULL, threadfunc, NULL);
  if(rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    return -1;
  }

  while(1){
    sleep(1);
    printf("I am a Main Thread\n");
  }

  return 0;
}