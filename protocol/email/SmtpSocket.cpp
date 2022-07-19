#include "SmtpSocket.h"
#include <sstream>
#include <time.h>
#include <string.h>
#include "Base64Util.h"
#include "Platform.h"

bool SmtpSocket::sendMail(const std::string& server, short port, const std::string& from, const std::string& fromPassword,
    const std::vector<std::string>& to, const std::string& subject, const std::string& mailData)
{
  size_t atSymbolPos = from.find_first_of('@');
  if (atSymbolPos == std::string::npos){
    return false;
  }

  std::string strUsr = from.substr(0, atSymbolPos);

  SmtpSocket smtpSocket;

  //smtp.126.com 25
  if(!smtpSocket.connect(server.c_str(), port)){
    return false;
  }

  if(!smtpSocket.logon(strUsr.c_str(), fromPassword.c_str())){
    return false;
  }

  if(!smtpSocket.sendMailFrom(from.c_str())){
    return false;
  }

  if(!smtpSocket.sendMailTo(to)){
    return false;
  }

  if(!smtpSocket.send(subject, mailData)){
    return false;
  }

  return true;
}

SmtpSocket::SmtpSocket(): m_bConnected(false), m_hSocket(INVALID_SOCKET)
{
}

SmtpSocket::~SmtpSocket()
{
  quit();
}

bool SmtpSocket::checkResponse(const char* recvCode)
{
  char recvBuf[1024] = {0};
  long lResult = 0;
  lResult = recv(m_hSocket, recvBuf, sizeof(recvBuf), 0);
  if (lResult == SOCKET_ERROR || lResult < 3){
    return false;
  }
  return recvCode[0] == recvBuf[0] && recvCode[1] == recvBuf[1] && recvCode[2] == recvBuf[2] ? true : false;  
}

void SmtpSocket::quit(){
  if(m_hSocket < 0){
    return;
  }
  //退出
  if(::send(m_hSocket, "QUIT\r\n", sizeof("QUIT\r\n"), 0) == SOCKET_ERROR){
    closeConnection();
    return;
  }

  if(!checkResponse("221")){
    return;
  }
}

bool SmtpSocket::logon(const char* pszUser, const char* pszPassword)
{
  if(m_hSocket < 0){
    return false;
  }

  //发送“AUTH LOGIN”
  if(::send(m_hSocket, "AUTH LOGIN\r\n", sizeof("AUTH LOGIN\r\n"), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("334")){
    return false;
  }

  //发送经base64编码的用户名
  char szUserEncoded[64] = {0};
  Base64Util::encode(szUserEncoded, pszUser, strlen(pszUser), '=', 64);
  strncat(szUserEncoded, "\r\n", sizeof(szUserEncoded));
  if(::send(m_hSocket, szUserEncoded, strlen(szUserEncoded), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("334")){
    return false;
  }

  //发送经base64编码的密码
  char szPwdEncoded[64] = {0};
  Base64Util::encode(szPwdEncoded, pszPassword, strlen(pszPassword), '=', 64);
  strncat(szPwdEncoded, "\r\n", sizeof(szPwdEncoded));
  if(::send(m_hSocket, szPwdEncoded, strlen(szPwdEncoded), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("235")){
    return false;
  }

  m_strUser = pszUser;
  m_strPassword = pszPassword;
  return true;
}

void SmtpSocket::closeConnection()
{
  if(m_hSocket < 0){
    return;
  }
  closesocket(m_hSocket);
  m_hSocket = INVALID_SOCKET;
  m_bConnected = false;
}

bool SmtpSocket::connect(const char* pszUrl, short port/* = 25*/)
{
  struct sockaddr_in server = {0};
  struct hostent* pHostent = NULL;
  unsigned int addr = 0;
  closeConnection();
  m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(m_hSocket == INVALID_SOCKET){
    return false;
  }
  long tmSend(15 * 1000L), tmRecv(15 * 1000L), noDelay(1);
  setsockopt(m_hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay, sizeof(noDelay));
  setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&tmSend, sizeof(tmSend));
  setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tmRecv, sizeof(tmRecv));

  if(inet_addr(pszUrl) == INADDR_NONE){
    pHostent = gethostbyname(pszUrl);
  }
  else{
    addr = inet_addr(pszUrl);
    pHostent = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
  }

  if(pHostent == NULL){
    return false;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = *((unsigned long*)pHostent->h_addr);
  server.sin_port = htons((u_short)port);

  if(::connect(m_hSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR){
    return false;
  }

  if(!checkResponse("220")){
    return false;
  }

  //向服务器发送"HELO”+服务器名
  char szSend[256] = {0};
  snprintf(szSend, sizeof(szSend), "HELO %s\r\n", pszUrl);
  if(::send(m_hSocket, szSend, strlen(szSend), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("250")){
    return false;
  }
  m_bConnected = true;
  return true;
}

bool SmtpSocket::sendMailFrom(const char* pszFrom)
{
  if(m_hSocket < 0){
    return false;
  }
  //发送“MAIL FROM:<from>”
  char szSend[256] = {0};
  snprintf(szSend, sizeof(szSend), "MAIL FROM:<%s>\r\n", pszFrom);
  if(::send(m_hSocket, szSend, strlen(szSend), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("250")){
    return false;
  }

  m_strFrom = pszFrom;
  return true;
}

bool SmtpSocket::sendMailTo(const std::vector<std::string> &sendTo)
{
  if(!m_hSocket < 0){
    return false;
  }
  //发送“RCPT TO:<to>”
  for(size_t i = 0; i < sendTo.size(); i++){
    char szSend[256] = {0};
    snprintf(szSend, sizeof(szSend), "rcpt tO:<%s>\r\n", sendTo[i].c_str());
    if(::send(m_hSocket, szSend, strlen(szSend), 0) == SOCKET_ERROR){
      return false;
    }
    if(!checkResponse("250")){
      return false;
    }
  }
  m_strTo = sendTo;
  return true;
}

bool SmtpSocket::send(const std::string& subject, const std::string& mailData)
{
  if(m_hSocket < 0){
    return false;
  }

  std::ostringstream osContent;

  //注意：邮件正文的内容与其他附属字样之间一定要空一行
  osContent << "Date: " << time(nullptr) << "\r\n";
  osContent << "from: " << m_strFrom << "\r\n";
  osContent << "to: ";
  for(const auto& iter : m_strTo){
    osContent << iter << ";";
  }
  osContent << "\r\n";
  osContent << "subject: " << subject << "\r\n";
  osContent << "Content-Type: text/plain; charset=utf-8\r\n";
  osContent << "Content-Transfer-Encoding: quoted-printable\r\n\r\n";
  osContent << mailData << "\r\n.\r\n";

  std::string data = osContent.str();
  const char* lpSendBuffer = data.c_str();

  //发送“DATA\r\n”
  if(::send(m_hSocket, "DATA\r\n", strlen("DATA\r\n"), 0) == SOCKET_ERROR){
    return false;
  }
  if(!checkResponse("354")){
    return false;
  }

  long dwSend = 0;
  long dwOffset = 0;
  long lTotal = data.length();
  long lResult = 0;
  const long SEND_MAX_SIZE = 1024 * 100000;
  while((long)dwOffset < lTotal){
    if(lTotal - dwOffset > SEND_MAX_SIZE){
      dwSend = SEND_MAX_SIZE;
    }
    else{
      dwSend = lTotal - dwOffset;
    }

    lResult = ::send(m_hSocket, lpSendBuffer + dwOffset, dwSend, 0);
    if(lResult == SOCKET_ERROR){
      return false;
    }

    dwOffset += lResult;
  }

  if(!checkResponse("250")){
    return false;
  }
}
