#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int main()
{
  pthread_mutex_t mymutex;
  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&mymutex, &mutex_attr);

  int ret = pthread_mutex_lock(&mymutex);
  printf("pthread_mutex_lock, ret: %d\n", ret);
  ret = pthread_mutex_lock(&mymutex);
  printf("pthread_mutex_lock, ret: %d\n", ret);
  if(ret == EDEADLK){
    printf("pthread_mutex_lock, EDEADLK\n");
  }

  pthread_mutex_destroy(&mymutex);
  pthread_mutexattr_destroy(&mutex_attr);

  return 0;
}