#include "TaskPool.h"
#include <chrono>

int main()
{
  TaskPool taskPool;
  taskPool.init(5);

  Task* task = NULL;
  for(int i = 0; i < 10; ++i){
    task = new Task();
    taskPool.addTask(task);
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  taskPool.stop();

  return 0;
}