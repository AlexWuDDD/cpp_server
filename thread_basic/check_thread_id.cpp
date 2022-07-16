#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

void* thread_proc(void* arg){
  pthread_t* tid1 = (pthread_t*)arg;
  int tid2 = syscall(SYS_gettid);
  pthread_t tid3 = pthread_self();

  while(true){
    printf("tid1: %ld\n", *tid1);
    printf("tid2: %d\n", tid2);
    printf("tid3: %ld\n", tid3);
    sleep(1);
  }
}

int main(){
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, thread_proc, &tid);
  if(rc){
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    return -1;
  }

  while(true){
    sleep(1);
    printf("I am a Main Thread\n");
  }

  pthread_join(tid, NULL);
  return 0;
}