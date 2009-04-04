
#include "stdafx.h"
#include "VTPFile.h"
#include "VTPSession.h"
#include "Util.h"
#include "URL.h"

#ifdef _LINUX
#define SD_BOTH SHUT_RDWR
#endif

using namespace XFILE;
using namespace std;

CVTPFile::CVTPFile()
  : m_socket(INVALID_SOCKET)
  , m_channel(0)
{
  m_session = new CVTPSession();
}

CVTPFile::~CVTPFile()
{
  Close();
}
void CVTPFile::Close()
{
  if(m_socket != INVALID_SOCKET)
  {
    m_session->AbortStreamLive();
    closesocket(m_socket);
  }

  m_session->Close();
  m_socket  = INVALID_SOCKET;
}

bool CVTPFile::Open(const CURL& url2)
{
  Close();
  CURL url(url2);
  if(url.GetHostName() == "")
    url.SetHostName("localhost");

  if(url.GetPort() == 0)
    url.SetPort(2004);

  CStdString path(url.GetFileName());

  if (path.Left(9) == "channels/")
  {

    CStdString channel = path.Mid(9);
    if(!CUtil::GetExtension(channel).Equals(".ts"))
    {
      CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
      return false;
    }
    CUtil::RemoveExtension(channel);

    if(!m_session->Open(url.GetHostName(), url.GetPort()))
      return false;

    m_channel = atoi(channel.c_str());
    m_socket  = m_session->GetStreamLive(m_channel);
  }
  else
  {
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, path.c_str());
    return false;
  }

  return true;
}

unsigned int CVTPFile::Read(void* buffer, __int64 size)
{
  if(m_socket == INVALID_SOCKET)
    return 0;

  fd_set         set_r, set_e;
  struct timeval tv;
  int            res;

  tv.tv_sec = 30;
  tv.tv_usec = 0;

  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket, &set_r);
  FD_SET(m_socket, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
  if(res < 0)
  {
    CLog::Log(LOGERROR, "CVTPFile::Read - select failed");
    return 0;
  }
  if(res == 0)
  {
    CLog::Log(LOGERROR, "CVTPFile::Read - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket, (char*)buffer, (size_t)size, 0);
  if(res < 0)
  {
    CLog::Log(LOGERROR, "CVTPFile::Read - failed");
    return 0;
  }
  if(res == 0)
  {
    CLog::Log(LOGERROR, "CVTPFile::Read - eof");
    return 0;
  }

  return res;
}

__int64 CVTPFile::Seek(__int64 pos, int whence)
{
  CLog::Log(LOGDEBUG, "CVTPFile::Seek - seek to pos %"PRId64", whence %d", pos, whence);

  if(whence == SEEK_POSSIBLE)
    return 0;

  return -1;
}

bool CVTPFile::NextChannel()
{
  if(m_session == NULL)
    return false;

  int channel = m_channel;
  while(++channel < 1000)
  {
    if(!m_session->CanStreamLive(channel))
      continue;

    if(m_socket != INVALID_SOCKET)
    {
      shutdown(m_socket, SD_BOTH);
      m_session->AbortStreamLive();
      closesocket(m_socket);
    }

    m_channel = channel;
    m_socket  = m_session->GetStreamLive(m_channel);
    if(m_socket != INVALID_SOCKET)
      return true;
  }
  return false;
}

bool CVTPFile::PrevChannel()
{
  if(m_session == NULL)
    return false;

  int channel = m_channel;
  while(--channel > 0)
  {
    if(!m_session->CanStreamLive(channel))
      continue;

    m_session->AbortStreamLive();

    if(m_socket != INVALID_SOCKET)
    {
      shutdown(m_socket, SD_BOTH);
      m_session->AbortStreamLive();
      closesocket(m_socket);
    }

    m_channel = channel;
    m_socket  = m_session->GetStreamLive(m_channel);
    if(m_socket != INVALID_SOCKET)
      return true;
  }
  return false;
}
