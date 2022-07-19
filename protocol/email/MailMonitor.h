#ifndef MAILMONITOR_H
#define MAILMONITOR_H

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

struct MailItem
{
  std::string subject;
  std::string content;
};

class MailMonitor
{
public:
  static MailMonitor& getInstance();

private:
  MailMonitor();
  ~MailMonitor();
  MailMonitor(const MailMonitor&) = delete;
  MailMonitor& operator=(const MailMonitor&) = delete;

public:
  bool initMonitorMainInfo(const std::string& servername, const std::string* mailserver, 
    short mailport, const std::string& mailfrom, const std::string& mailfromPassword, const std::string& mailto);
  void uninit();
  void wait();
  void run();
  bool alert(const std::string& subject, const std::string& content);

private:
  void alertThread();
  void split(const std::string& str, std::vector<std::string>& v, const char* delimiter = "|");

private:
  //use to distinguish different mail server
  std::string m_strMailName;
  std::string m_strMailServer;
  short m_nMailPort;
  std::string m_strFrom;
  std::string m_strFromPassword;
  std::vector<std::string> m_strMailTo;

  std::list<MailItem> m_listMainItemToSend;
  std::shared_ptr<std::thread> m_spMailAlertThread;
  std::mutex m_mutexAlert;
  std::condition_variable m_cvAlert;

  bool m_bExit;
  bool m_bRunning;

};





#endif