/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "threads/SystemClock.h"
#include "PlatformInclude.h"
#include "XLCDproc.h"
#include "../utils/log.h"
#include "../utils/TimeUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SCROLL_SPEED_IN_MSEC 250


XLCDproc::XLCDproc()
{
  m_iActualpos   = 0;
  m_iBackLight   = 32;
  m_iLCDContrast = 50;
  m_iColumns     = 0;
  m_iRows        = 0;
  m_bStop        = true;
  m_sockfd       = -1;
  m_lastInitAttempt = 0;
  m_initRetryInterval = INIT_RETRY_INTERVAL;
  m_used = true;
}

XLCDproc::~XLCDproc()
{
}

void XLCDproc::Initialize()
{
  if (!m_used || !g_guiSettings.GetBool("videoscreen.haslcd"))
    return ;//nothing to do

  // don't try to initialize too often
  int now = XbmcThreads::SystemClockMillis();
  if ((now - m_lastInitAttempt) < m_initRetryInterval)
    return;
  m_lastInitAttempt = now;

  ILCD::Initialize();

  if (Connect())
  {
    // reset the retry interval after a successful connect
    m_initRetryInterval = INIT_RETRY_INTERVAL;

    m_bStop = false;
  }
  else
  {
    CloseSocket();

    // give up after 60 seconds
    if (m_initRetryInterval > INIT_RETRY_INTERVAL_MAX)
    {
      m_used = false;
      CLog::Log(LOGERROR, "XLCDproc::%s - Connect failed. Giving up.", __FUNCTION__);
    }
    else
    {
      m_initRetryInterval *= 2;
      CLog::Log(LOGERROR, "XLCDproc::%s - Connect failed. Retry in %d seconds.", __FUNCTION__,
                m_initRetryInterval/1000);
    }
  }
}

bool XLCDproc::Connect()
{
  CloseSocket();

  struct hostent *server;
  server = gethostbyname(g_advancedSettings.m_lcdHostName);
  if (server == NULL)
  {
     CLog::Log(LOGERROR, "XLCDproc::%s - Unable to resolve LCDd host.", __FUNCTION__);
     return false;
  }

  m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_sockfd == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to create socket.", __FUNCTION__);
    return false;
  }

  struct sockaddr_in serv_addr = {};
  serv_addr.sin_family = AF_INET;
  memmove(&serv_addr.sin_addr, server->h_addr_list[0], server->h_length);
  //Connect to default LCDd port, hard coded for now.
  serv_addr.sin_port = htons(13666);

  if (connect(m_sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to connect to host, LCDd not running?", __FUNCTION__);
    return false;
  }

  // Start a new session
  CStdString hello;
  hello = "hello\n";

  if (write(m_sockfd,hello.c_str(),hello.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    return false;
  }

  // Receive LCDproc data to determine row and column information
  char reply[1024];
  if (read(m_sockfd,reply,1024) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket", __FUNCTION__);
    return false;
  }

  unsigned int i=0;
  while ((strncmp("lcd",reply + i,3) != 0 ) && (i < (strlen(reply) - 5))) i++;
  if(sscanf(reply+i,"lcd wid %u hgt %u", &m_iColumns, &m_iRows))
    CLog::Log(LOGDEBUG, "XLCDproc::%s - LCDproc data: Columns %i - Rows %i.", __FUNCTION__, m_iColumns, m_iRows);

  //Build command to setup screen
  CStdString cmd;
  cmd = "screen_add xbmc\n";
  if (!g_advancedSettings.m_lcdHeartbeat)
    cmd.append("screen_set xbmc -heartbeat off\n");
  if (g_advancedSettings.m_lcdScrolldelay != 0)
  {
    cmd.append("widget_add xbmc line1 scroller\n");
    cmd.append("widget_add xbmc line2 scroller\n");
    cmd.append("widget_add xbmc line3 scroller\n");
    cmd.append("widget_add xbmc line4 scroller\n");
  }
  else
  {
    cmd.append("widget_add xbmc line1 string\n");
    cmd.append("widget_add xbmc line2 string\n");
    cmd.append("widget_add xbmc line3 string\n");
    cmd.append("widget_add xbmc line4 string\n");
  }

  //Send to server
  if (write(m_sockfd,cmd.c_str(),cmd.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    return false;
  }

  return true;
}

void XLCDproc::CloseSocket()
{
  if (m_sockfd != -1)
  {
    shutdown(m_sockfd, SHUT_RDWR);
    close(m_sockfd);
    m_sockfd = -1;
  }
}

bool XLCDproc::IsConnected()
{
  if (m_sockfd == -1)
    return false;

  CStdString cmd;
  cmd = "noop\n";

  if (write(m_sockfd,cmd.c_str(),cmd.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    CloseSocket();
    return false;
  }

  return true;
}

void XLCDproc::SetBackLight(int iLight)
{
  if (m_sockfd == -1)
    return;

  //Build command
  CStdString cmd;

  if (iLight == 0)
  {
    m_bStop = true;
    cmd = "screen_set xbmc -backlight off\n";
    cmd.append("widget_del xbmc line1\n");
    cmd.append("widget_del xbmc line2\n");
    cmd.append("widget_del xbmc line3\n");
    cmd.append("widget_del xbmc line4\n");
  }
  else if (iLight > 0)
  {
    m_bStop = false;
    cmd = "screen_set xbmc -backlight on\n";
    if (g_advancedSettings.m_lcdScrolldelay != 0)
    {
      cmd.append("widget_add xbmc line1 scroller\n");
      cmd.append("widget_add xbmc line2 scroller\n");
      cmd.append("widget_add xbmc line3 scroller\n");
      cmd.append("widget_add xbmc line4 scroller\n");
    }
    else
    {
      cmd.append("widget_add xbmc line1 string\n");
      cmd.append("widget_add xbmc line2 string\n");
      cmd.append("widget_add xbmc line3 string\n");
      cmd.append("widget_add xbmc line4 string\n");
    }
  }

  //Send to server
  if (write(m_sockfd,cmd.c_str(),cmd.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    CloseSocket();
  }
}
void XLCDproc::SetContrast(int iContrast)
{
  //TODO: Not sure if you can control contrast from client
}

void XLCDproc::Stop()
{
  CloseSocket();
  m_bStop = true;
}

void XLCDproc::Suspend()
{
  if (m_bStop || m_sockfd == -1)
    return;

  //Build command to suspend screen
  CStdString cmd;
  cmd = "screen_set xbmc -priority hidden\n";

  //Send to server
  if (write(m_sockfd,cmd.c_str(),cmd.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    CloseSocket();
  }
}

void XLCDproc::Resume()
{
  if (m_bStop || m_sockfd == -1)
    return;

  //Build command to resume screen
  CStdString cmd;
  cmd = "screen_set xbmc -priority info\n";

  //Send to server
  if (write(m_sockfd,cmd.c_str(),cmd.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    CloseSocket();
  }
}

int XLCDproc::GetColumns()
{
  return m_iColumns;
}

int XLCDproc::GetRows()
{
  return m_iRows;
}

void XLCDproc::SetLine(int iLine, const CStdString& strLine)
{
  if (m_bStop || m_sockfd == -1)
    return;

  if (iLine < 0 || iLine >= (int)m_iRows)
    return;

  CStdString strLineLong = strLine;
  strLineLong.Trim();
  StringToLCDCharSet(strLineLong);

  //make string fit the display if it's smaller than the width
  if (strLineLong.size() < m_iColumns)
    strLineLong.append(m_iColumns - strLineLong.size(), ' ');
  //else if the string doesn't fit the display, lcdproc will scroll it, so we need a space
  else if (strLineLong.size() > m_iColumns)
    strLineLong += " ";

  if (strLineLong != m_strLine[iLine])
  {
    CStdString cmd;
    int ln = iLine + 1;

    if (g_advancedSettings.m_lcdScrolldelay != 0)
      cmd.Format("widget_set xbmc line%i 1 %i %i %i m %i \"%s\"\n", ln, ln, m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay, strLineLong.c_str());
    else
      cmd.Format("widget_set xbmc line%i 1 %i \"%s\"\n", ln, ln, strLineLong.c_str());

    if (write(m_sockfd, cmd.c_str(), cmd.size()) == -1)
    {
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
      CloseSocket();
      return;
    }
    m_bUpdate[iLine] = true;
    m_strLine[iLine] = strLineLong;
    m_event.Set();
  }
}

void XLCDproc::Process()
{
}

