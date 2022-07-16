#include <pthread.h>
#include <unistd.h>
#include <iostream>

int resourceID = 0;
pthread_rwlock_t myrwlock;

void* read_thread(void* param)
{
  while(true){
    //request read lock
    pthread_rwlock_rdlock(&myrwlock);
    std::cout << "read thread ID: " << pthread_self() << " ,resourceID: " << resourceID << std::endl;

    //use sleep to simulate the time of reading
    sleep(1);

    //release read lock
    pthread_rwlock_unlock(&myrwlock);
  }
  return NULL;
}

void* write_thread(void* param){
  while(true){
    //request write lock
    pthread_rwlock_wrlock(&myrwlock);

    ++resourceID;
    std::cout << "write thread ID: " << pthread_self() << " ,resourceID: " << resourceID << std::endl;

    //use sleep to simulate the time of writing

    pthread_rwlock_unlock(&myrwlock);

    sleep(5);
  }

  return NULL;
}

int main(){

  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init(&attr);
  //set the write lock has the priority over the read lock
  pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

  pthread_rwlock_init(&myrwlock, &attr);

  pthread_t readThreadID[5];
  for(int i = 0; i < 5; ++i){
    pthread_create(&readThreadID[i], NULL, read_thread, NULL);
  }

  pthread_t writeThreadID;
  pthread_create(&writeThreadID, NULL, write_thread, NULL);

  pthread_join(writeThreadID, NULL);

  for(int i = 0; i < 5; ++i){
    pthread_join(readThreadID[i], NULL);
  }

  pthread_rwlock_destroy(&myrwlock);
  pthread_rwlockattr_destroy(&attr);

  return 0;
}