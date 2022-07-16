#include "MyThread.h"

int main(){
  MyThread myThread;
  myThread.Start();

  while(1){
    printf("main thread\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}