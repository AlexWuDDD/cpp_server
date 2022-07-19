#include "MailMonitor.h"
#include <functional>
#include <sstream>
#include <iostream>
#include <string.h>
#include "SmtpSocket.h"

MailMonitor& MailMonitor::getInstance()
{
  static MailMonitor instance;
  return instance;
}

bool MailMonitor::initMonitorMainInfo(const std::string& servername, const std::string* mailserver, 
  short mailport, const std::string& mailfrom, const std::string& mailfromPassword, const std::string& mailto)
{
  m_strMailName = servername;
  m_strMailServer = *mailserver;
  m_nMailPort = mailport;
  m_strFrom = mailfrom;
  m_strFromPassword = mailfromPassword;
  split(mailto, m_strMailTo, ";");
  std::ostringstream osSubject;
  osSubject << "[" << m_strMailName << "]";

  SmtpSocket::sendMail(m_strMailServer, m_nMailPort, m_strFrom, m_strFromPassword, m_strMailTo, osSubject.str(), "You have started Mail ALert System.");

  return true;
}


void MailMonitor::uninit()
{
  m_bExit = true;
  m_cvAlert.notify_one();
  if(m_spMailAlertThread->joinable())
  {
    m_spMailAlertThread->join();
  }
}

void MailMonitor::wait()
{
  if(m_spMailAlertThread->joinable())
  {
    m_spMailAlertThread->join();
  }
}

void MailMonitor::run()
{
  m_spMailAlertThread.reset(new std::thread(std::bind(&MailMonitor::alertThread, this)));
}


void MailMonitor::alertThread(){

  m_bRunning = true;
  while(true){
    MailItem mailItem;
    {
      std::unique_lock<std::mutex> guard(m_mutexAlert);
      while(m_listMainItemToSend.empty()){
        if(m_bExit){
          return;
        }
        m_cvAlert.wait(guard);
      }

      mailItem = m_listMainItemToSend.front();
      m_listMainItemToSend.pop_front();
    }

    std::ostringstream osSubject;
    osSubject << "[" << m_strMailName << "]" << mailItem.subject;
    SmtpSocket::sendMail(m_strMailServer, m_nMailPort, m_strFrom, m_strFromPassword, m_strMailTo, osSubject.str(), mailItem.content);
  }
  m_bRunning = false;
}

bool MailMonitor::alert(const std::string& subject, const std::string& content)
{
  if(m_strMailServer.empty() || m_nMailPort < 0 || m_strFrom.empty() || m_strFromPassword.empty() || m_strMailTo.empty()){
    return false;
  }

  MailItem mailItem;
  mailItem.subject = subject;
  mailItem.content = content;
  {
    std::lock_guard<std::mutex> guard(m_mutexAlert);
    m_listMainItemToSend.push_back(mailItem);
  }
  m_cvAlert.notify_one();
  return true;
}

void MailMonitor::split(const std::string& str, std::vector<std::string>& v, const char* delimiter)
{
  if(delimiter == nullptr || str.empty()){
    return;
  }

  std::string buf(str);
  size_t pos = std::string::npos;
  std::string substr;
  int delimiterLen = strlen(delimiter);
  while(true){
    pos = buf.find(delimiter);
    if(pos != std::string::npos){
      substr = buf.substr(0, pos);
      if(!substr.empty()){
        v.push_back(substr);
      }

      buf = buf.substr(pos + delimiterLen);
    }
    else{
      if(!buf.empty()){
        v.push_back(buf);
      }
      break;
    }
  }
}