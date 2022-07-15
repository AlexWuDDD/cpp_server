#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

pthread_mutex_t mymutex;

void *worker_thread(void * param)
{
  pthread_t threadID = pthread_self();
  printf("thread start, ThreadID: %ld\n", threadID);

  while(true){
    int ret = pthread_mutex_lock(&mymutex);
    if(ret == EDEADLK){
      printf("EDEADLK, ThreadID: %ld\n", threadID);
    }
    else{
      printf("ret = %d, ThreadID: %ld\n", ret, threadID);
    }

    sleep(1);
  }
  return NULL;
}

int main()
{
  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&mymutex, &mutex_attr);

  //create 5 threads
  pthread_t threadID[5];
  for(int i = 0; i < 5; ++i){
    pthread_create(&threadID[i], NULL, worker_thread, NULL);
  }

  for(int i = 0; i < 5; ++i){
    pthread_join(threadID[i], NULL);
  }

  pthread_mutex_destroy(&mymutex);
  pthread_mutexattr_destroy(&mutex_attr);

  return 0;
}