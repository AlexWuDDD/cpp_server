#ifndef __IULOG_H__
#define __IULOG_H__

enum LOG_LEVEL
{
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
};

#define LOG_INFO(...) CIULog::Log(LOG_LEVEL_INFO, __FUNCSIG__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) CIULog::Log(LOG_LEVEL_WARNING, __FUNCSIG__ __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) CIULog::Log(LOG_LEVEL_ERROR, __FUNCSIG__, __LINE__, __VA_ARGS__)

class CIULog
{
public:
  static bool Init(bool bToFile, bool bTruncateLongLog, const char* pszLogFileName);
  static void Uninit();

  static void SetLevel(LOG_LEVEL nLevel);

  static bool Log(long nLevel, const char* pszFormat, ...);
  static bool Log(long nLevel, const char* pszFunctionSig, int nLineNo, const char* pszFormat, ...);

private:
  CIULog() = delete;
  ~CIULog() = delete;

  CIULog(const CIULog&) = delete;
  CIULog& operator=(const CIULog&) = delete;

  static void GetTime(char* pszTime, int nTimeStrLength);

private:
  static bool m_bToFile;
  static int m_hLogFile;
  static bool m_bTruncateLongLog;
  static LOG_LEVEL m_nLevel;
};

#endif