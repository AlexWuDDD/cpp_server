#include "TaskPool.h"

TaskPool::TaskPool() : m_bRunning(false)
{
}

TaskPool::~TaskPool()
{
  removeAllTasks();
}

void TaskPool::init(int threadNum)
{
  if(threadNum < 5){
    threadNum = 5;
  }

  m_bRunning = true;

  for(int i = 0; i < threadNum; ++i){
    std::shared_ptr<std::thread> spThread;
    spThread.reset(new std::thread(&TaskPool::threadFunc, this));
    m_threads.push_back(spThread);
  }
}

void TaskPool::threadFunc()
{
  std::shared_ptr<Task> spTask;
  while(true){
    {
      std::unique_lock<std::mutex> guard(m_mutexList);
      while(m_taskList.empty()){
        if(!m_bRunning)
          break;
        m_cv.wait(guard);
      }

      if(!m_bRunning)
        break;

      spTask = m_taskList.front();
      m_taskList.pop_front();
    }

    if(spTask == NULL){
      break;
    }

    spTask->doIt();
    spTask.reset();
  }

  std::cout << "exit thread, threadID: " << std::this_thread::get_id() << std::endl;
}

void TaskPool::stop()
{
  m_bRunning = false;
  m_cv.notify_all();
  for(auto& iter : m_threads){
    if(iter->joinable()){
      iter->join();
    }
  }
}

void TaskPool::addTask(Task* task)
{
  std::shared_ptr<Task> spTask(task);
  {
    std::unique_lock<std::mutex> guard(m_mutexList);
    m_taskList.push_back(spTask);
    std::cout << "add a Task" << std::endl;
  }
  m_cv.notify_one();
}

void TaskPool::removeAllTasks()
{
  std::unique_lock<std::mutex> guard(m_mutexList);
  for(auto& iter : m_taskList){
    iter.reset();
  }
  m_taskList.clear();
}
