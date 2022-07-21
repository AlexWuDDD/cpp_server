#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <stdint.h>
#include <time.h>
#include <mutex>

typedef std::function<void()> TimerCallback;

class Timer
{
public:
  Timer(int32_t repeatedTimes, int64_t interval, const TimerCallback& callback)
  {
    m_repeatedTimes = repeatedTimes;
    m_interval = interval;
    m_callback = callback;

    m_expiredTime = (int64_t)time(nullptr) + interval;
    m_id = Timer::generatedId();
  }
  ~Timer();

  int64_t getId() const
  {
    return m_id;
  }

  bool isExpired()
  {
    int64_t now = (int64_t)time(nullptr);
    return now >= m_expiredTime;
  }

  int32_t getRepeatedTimes() const
  {
    return m_repeatedTimes;
  }

  int64_t getExpiredTime() const
  {
    return m_expiredTime;
  }

  static int64_t generatedId(){
    int64_t tmpId;
    s_mutex.lock();
    ++s_initialId;
    tmpId = s_initialId;
    s_mutex.unlock();

    return tmpId;
  }

  void run(){
    m_callback();
    if(m_repeatedTimes >= 1){
      --m_repeatedTimes;
    }
    m_expiredTime += m_interval;
  }

private:
  static int64_t s_initialId {0};
  static std::mutex s_mutex;

private:
  int64_t       m_id; 			//定时器ID，唯一标识一个定时器
  time_t        m_expiredTime;		//定时器的到期时间
  int64_t       m_interval;		//定时器的间隔时间
  int32_t       m_repeatedTimes;        //定时器重复触发的次数
  TimerCallback m_callback;             //定时器触发后的回调函数
  /*m_callback中不能有耗时或者阻塞的操作，如果有需要移到其他线程中*/
};


#endif
