#include <thread>
#include <mutex>
#include <list>
#include <string>
#include <sstream>
#include <iostream>
#include <condition_variable>

std::mutex log_mutex;
std::condition_variable log_cv;
std::list<std::string> cached_logs;
FILE* log_file;

bool init_log_file()
{
  log_file = fopen("my.log", "a+");
  return log_file != nullptr;
}

void uninit_log_file()
{
  if(log_file != nullptr){
    fclose(log_file);
  }
}

bool write_log_tofile(const std::string& line)
{
  if(log_file == nullptr){
    return false;
  }
  if(fwrite((void*)line.c_str(), 1, line.length(), log_file) != line.length()){
    return false;
  }

  fflush(log_file);

  return true;
}

void log_producer()
{
  int index = 0;
  while(true){
    ++index;
    std::ostringstream os;
    os << "This is log, index: " << index << ", produced by thread: " << std::this_thread::get_id() << std::endl;

    {
      std::lock_guard<std::mutex> lock(log_mutex);
      cached_logs.emplace_back(os.str());
      log_cv.notify_one();
    }

    std::chrono::milliseconds duration(100);
    std::this_thread::sleep_for(duration);
  }
}

void log_consumer()
{
  std::string line;
  while(true){
    {
      std::unique_lock<std::mutex> lock(log_mutex);
      while(cached_logs.empty()){
        log_cv.wait(lock);
      }

      line = cached_logs.front();
      cached_logs.pop_front();
    }

    if(line.empty()){
      std::chrono::milliseconds duration(100);
      std::this_thread::sleep_for(duration);
      continue;
    }
    write_log_tofile(line);
    line.clear();
  }
}

int main(int argc, char* argv[])
{
  if(!init_log_file()){
    std::cout << "init log file failed" << std::endl;
    return -1;
  }
  std::thread producer_thread1(log_producer);
  std::thread producer_thread2(log_producer);
  std::thread producer_thread3(log_producer);

  std::thread consumer_thread1(log_consumer);
  std::thread consumer_thread2(log_consumer);
  std::thread consumer_thread3(log_consumer);
  
  producer_thread1.join();
  producer_thread2.join();
  producer_thread3.join();

  consumer_thread1.join();
  consumer_thread2.join();
  consumer_thread3.join();

  uninit_log_file();
  return 0;
}