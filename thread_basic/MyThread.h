#include <thread>
#include <memory>
#include <stdio.h>

class MyThread
{
public:
  MyThread()
  {

  }
  ~MyThread()
  {

  }

  void Start()
  {
    m_stopped = false;
    //threadFunc is not static function, so we need to pass this as a parameter
    m_spThread.reset(new std::thread(&MyThread::threadFunc, this, 8888, 9999));
  }

  void Stop()
  {
    m_stopped = true;
    if(m_spThread)
    {
      if(m_spThread->joinable())
      {
        m_spThread->join();
      }
    }
  }

private:

  void threadFunc(int arg1, int arg2){
    while(!m_stopped)
    {
      printf("threadFunc: %d, %d\n", arg1, arg2);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  // void* threadFunc(void *arg);

private:
  std::shared_ptr<std::thread> m_spThread;
  bool m_stopped;
};