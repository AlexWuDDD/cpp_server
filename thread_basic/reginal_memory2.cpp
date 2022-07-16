#include<pthread.h>
#include <iostream>
#include <unistd.h>

//thread local variable
__thread int g_mydata = 99;

void* thread_function1(void* args)
{
  while(true){
    ++g_mydata; //belong to thread_function1 only
  }
  return NULL;
}

void *thread_function2(void* args)
{
  while(true){
    //belong to thread_function2 only
    std::cout << "g_mydata: " << g_mydata << ", ThreadID: " << pthread_self() << std::endl;
    sleep(1);
  }

  return NULL;
}

int main()
{
  pthread_t threadIDs[2];
  pthread_create(&threadIDs[0], NULL, thread_function1, NULL);
  pthread_create(&threadIDs[1], NULL, thread_function2, NULL);

  for(int i = 0; i < 2; i++){
    pthread_join(threadIDs[i], NULL);
  }

  return 0;
}