
#include "stdafx.h"
#include "VTPFile.h"
#include "VTPSession.h"
#include "Util.h"
#include "URL.h"


using namespace XFILE;
using namespace std;

CVTPFile::CVTPFile()
  : m_socket(INVALID_SOCKET)
{
  m_session = new CVTPSession();
}

CVTPFile::~CVTPFile()
{
  Close();
}
void CVTPFile::Close()
{
  m_session->Close();

  if(m_socket != INVALID_SOCKET)
    closesocket(m_socket);
  m_socket  = INVALID_SOCKET;
}

bool CVTPFile::Open(const CURL& url2, bool binary)
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

    m_socket = m_session->GetStreamLive(atoi(channel.c_str()));
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

  int res;
  res = recv(m_socket, buffer, (size_t)size, 0);
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
