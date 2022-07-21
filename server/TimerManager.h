#ifndef __TIMER_MANAGER_H__
#define __TIMER_MANAGER_H__

#include <list>
#include <stdint.h>
#include "Timer.h"

struct TimerCompare
{
  bool operator()(const Timer* lhs, const Timer* rhs)
  {
    return lhs->getExpiredTime() < rhs->getExpiredTime();
  }
};

class TimerManager
{
public:
  TimerManager() = default;
  ~TimerManager() = default;

  int64_t addTimer(int32_t repeatedTimes, int64_t interval, const TimerCallback& callback)
  {
    Timer* pTimer = new Timer(repeatedTimes, interval, callback);
    m_listTimers.push_back(pTimer);
    m_listTimers.sort(TimerCompare());
    return pTimer->getId();
  }

  bool removeTimer(int64_t timerId)
  {
    for (auto it = m_listTimers.begin(); it != m_listTimers.end(); ++it)
    {
      if ((*it)->getId() == timerId)
      {
        m_listTimers.erase(it);
        return true;
      }
    }
    return false;
  }

  void checkAndHandleTimers()
  {
    bool adjusted = false;
    Timer* deltedTimer;
    for (auto it = m_listTimers.begin(); it != m_listTimers.end();){
      if ((*it)->isExpired()){
        (*it)->run();
        if ((*it)->getRepeatedTimes() == 0)
        {
          deltedTimer = *it;
          it = m_listTimers.erase(it);
          delete deltedTimer;
        }
        else
        {
          ++it;
          adjusted = true;
        }
      }
      else{
        break;
      }
    }

    if (adjusted)
    {
      m_listTimers.sort(TimerCompare());
    }
  }

private:
  std::list<Timer*> m_listTimers;
};



#endif