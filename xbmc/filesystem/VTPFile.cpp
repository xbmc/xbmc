/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "VTPFile.h"
#include "VTPSession.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

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
  delete m_session;
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
    if(!URIUtils::GetExtension(channel).Equals(".ts"))
    {
      CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
      return false;
    }
    URIUtils::RemoveExtension(channel);

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

unsigned int CVTPFile::Read(void* buffer, int64_t size)
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

int64_t CVTPFile::Seek(int64_t pos, int whence)
{
  CLog::Log(LOGDEBUG, "CVTPFile::Seek - seek to pos %"PRId64", whence %d", pos, whence);
  return -1;
}

bool CVTPFile::NextChannel(bool preview/* = false*/)
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
      shutdown(m_socket, SHUT_RDWR);
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

bool CVTPFile::PrevChannel(bool preview/* = false*/)
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
      shutdown(m_socket, SHUT_RDWR);
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

bool CVTPFile::SelectChannel(unsigned int channel)
{
  if(!m_session->CanStreamLive(channel))
    return false;

  m_session->AbortStreamLive();

  if(m_socket != INVALID_SOCKET)
  {
    shutdown(m_socket, SHUT_RDWR);
    m_session->AbortStreamLive();
    closesocket(m_socket);
  }

  m_channel = channel;
  m_socket  = m_session->GetStreamLive(m_channel);
  if(m_socket != INVALID_SOCKET)
    return true;
  else
    return false;
}

int CVTPFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 0;

  return -1;
}
