#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main()
{
  pthread_mutex_t mymutex;
  pthread_mutex_init(&mymutex, NULL);

  int ret = pthread_mutex_lock(&mymutex);
  
  //try to destory the locked mutex object
  ret = pthread_mutex_destroy(&mymutex);
  if(ret != 0){
    if(ret == EBUSY){
      printf("%s- %d\n", strerror(ret), ret);
    }
    printf("Failed to destory the mutex object\n");
  }

  ret = pthread_mutex_unlock(&mymutex);
  ret = pthread_mutex_destroy(&mymutex);
  if(ret == 0){
    printf("Destory the mutex object Successfully\n");
  }
}