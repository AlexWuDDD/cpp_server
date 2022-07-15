#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

std::mutex mymutex;
std::condition_variable mycv;
bool success = false;

void thread_func()
{
  {
    std::unique_lock<std::mutex> lock(mymutex);
    success = true;
    mycv.notify_one();
  }

  while(true){

  }
}

int main()
{
  std::thread t(thread_func);
  
  {
    std::unique_lock<std::mutex> lock(mymutex);
    while(!success){
      mycv.wait(lock);
    }
  }
  std::cout << "start thread successfully." << std::endl;

  t.join();

  return 0;
}