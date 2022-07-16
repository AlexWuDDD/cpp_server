#include <pthread.h>
#include <unistd.h>
#include <list>
#include <semaphore.h>
#include <iostream>

class Task{
public:
  Task(int taskID){
    this->taskID = taskID;
  }
  void doTask(){
    std::cout << "handle a task, taskID: " << taskID << ", threadID: " << pthread_self() << std::endl;
  }

private:
  int taskID;
};

pthread_mutex_t mymutex;
std::list<Task*> tasks;
pthread_cond_t mycv;

void* consumer_thread(void* param)
{
  Task* pTask = NULL;
  while(true){
    pthread_mutex_lock(&mymutex);
    while(tasks.empty()){
      //if get mutex lock, but the condition is not satisfied, 
      //then pthread_cond_wait unlock the mutex and will not execute the code
      //but if the condition is satisfied, pthread_cond_wait will get the lock
      pthread_cond_wait(&mycv, &mymutex);
    }

    pTask = tasks.front();
    tasks.pop_front();

    pthread_mutex_unlock(&mymutex);

    if(pTask == NULL){
      continue;
    }

    pTask->doTask();
    delete pTask;
    pTask = NULL;
  }

  return NULL;
} 

void* producer_thread(void* param)
{
  int taskID = 0;
  Task* pTask = NULL;

  while(true){
    pTask = new Task(taskID);
    pthread_mutex_lock(&mymutex);
    tasks.push_back(pTask);
    std::cout << "produce a task, taskID: " << taskID << ", threadID: " << pthread_self() << std::endl;
    pthread_mutex_unlock(&mymutex);

    //pthread_cond_signal will wake up one thread that is waiting on the condition
    pthread_cond_signal(&mycv);
    
    taskID++;
    sleep(1);
  }
  return NULL;
}

int main()
{
  pthread_mutex_init(&mymutex, NULL);
  pthread_cond_init(&mycv, NULL);

  pthread_t consumerThreadID[5];
  for(int i = 0; i < 5 ; ++i){
    pthread_create(&consumerThreadID[i], NULL, consumer_thread, NULL);
  }

  pthread_t producerThreadID;
  pthread_create(&producerThreadID, NULL, producer_thread, NULL);

  pthread_join(producerThreadID, NULL);

  for(int i = 0; i < 5 ; ++i){
    pthread_join(consumerThreadID[i], NULL);
  }

  pthread_cond_destroy(&mycv);
  pthread_mutex_destroy(&mymutex);

  return 0;
}