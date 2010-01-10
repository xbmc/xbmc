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

#include "PlatformInclude.h"
#include "XLCDproc.h"
#include "../utils/log.h"
#include "../utils/TimeUtils.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"

#define SCROLL_SPEED_IN_MSEC 250


XLCDproc::XLCDproc()
{
  m_iActualpos   = 0;
  m_iBackLight   = 32;
  m_iLCDContrast = 50;
  m_bStop	       = true;
  sockfd         = -1;
  m_lastInitAttempt = 0;
  m_initRetryInterval = INIT_RETRY_INTERVAL;
  m_used = true;
}

XLCDproc::~XLCDproc()
{
}

void XLCDproc::Initialize()
{
  if (!g_guiSettings.GetBool("videoscreen.haslcd"))
    return ;//nothing to do

  // don't try to initialize too often
  int now = CTimeUtils::GetTimeMS();
  if (!m_used || now < m_lastInitAttempt + m_initRetryInterval)
    return;
  m_lastInitAttempt = now;

  ILCD::Initialize();

  struct hostent *server;
  server = gethostbyname(g_advancedSettings.m_lcdHostName);
  if (server == NULL)
  {
     CLog::Log(LOGERROR, "XLCDproc::%s - Unable to resolve LCDd host.", __FUNCTION__);
     return;
  }

  struct sockaddr_in serv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  memmove((char *)&serv_addr.sin_addr,
          (char *)server->h_addr_list[0],
          server->h_length);

  //Connect to default LCDd port, hard coded for now.

  serv_addr.sin_port = htons(13666);
  if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
  {
    // give up after 60 seconds
    if (m_initRetryInterval > INIT_RETRY_INTERVAL_MAX)
    {
      m_used = false;
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to connect to host. Giving up.", __FUNCTION__);
    }
    else
    {
      m_initRetryInterval *= 2;
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to connect to host. Retry in %d seconds.", __FUNCTION__,
                m_initRetryInterval/1000);
    }

    return;
  }

  // reset the retry interval after a successful connect
  m_initRetryInterval = INIT_RETRY_INTERVAL;

  // Start a new session
  CStdString hello;
  hello = "hello\n";

  if (write(sockfd,hello.c_str(),hello.size()) < 0)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    return;
  }

  // Receive LCDproc data to determine row and column information
  char reply[1024];
  unsigned int i=0;

  if (read(sockfd,reply,1024) < 0)
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket", __FUNCTION__);

  while ((strncmp("lcd",reply+i,3) != 0 ) && (i<(strlen(reply)-5))) i++;
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
  if (write(sockfd,cmd.c_str(),cmd.size()) < 0)
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
  m_bStop = false;
}

bool XLCDproc::IsConnected()
{
  CStdString cmd;
  cmd = "noop\n";

  if (write(sockfd,cmd.c_str(),cmd.size()) >= 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void XLCDproc::SetBackLight(int iLight)
{
  if (sockfd > 0)
  {
    //Build command
    CStdString cmd;

    if (iLight == 0) {
      m_bStop = true;
      cmd = "screen_set xbmc -backlight off\n";
      cmd.append("widget_del xbmc line1\n");
      cmd.append("widget_del xbmc line2\n");
      cmd.append("widget_del xbmc line3\n");
      cmd.append("widget_del xbmc line4\n");
    }
    if (iLight > 0) {
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
    if (write(sockfd,cmd.c_str(),cmd.size()) < 0)
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
  }
}
void XLCDproc::SetContrast(int iContrast)
{
  //TODO: Not sure if you can control contrast from client
}

void XLCDproc::Stop()
{
  //Close connection
  if (sockfd >= 0)
    shutdown(sockfd, SHUT_RDWR);
  m_bStop = true;
}

void XLCDproc::Suspend()
{
  if (!m_bStop)
  {
    if (sockfd > 0)
    {
      //Build command to suspend screen
      CStdString cmd;
      cmd = "screen_set xbmc -priority hidden\n";

      //Send to server
      if (write(sockfd,cmd.c_str(),cmd.size()) < 0)
        CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    }
  }
}

void XLCDproc::Resume()
{
  if (!m_bStop)
  {
    if (sockfd > 0)
    {
      //Build command to resume screen
      CStdString cmd;
      cmd = "screen_set xbmc -priority info\n";

      //Send to server
      if (write(sockfd,cmd.c_str(),cmd.size()) < 0)
        CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    }
  }
}

void XLCDproc::SetLine(int iLine, const CStdString& strLine)
{
  if (m_bStop)
    return;

  if (iLine < 0 || iLine >= (int)m_iRows)
    return;

  CStdString cmd;
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
    int ln = iLine + 1;

    if (g_advancedSettings.m_lcdScrolldelay != 0)
      cmd.Format("widget_set xbmc line%i 1 %i %i %i m %i \"%s\"\n", ln, ln, m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay, strLineLong.c_str());
    else
      cmd.Format("widget_set xbmc line%i 1 %i \"%s\"\n", ln, ln, strLineLong.c_str());

    if (write(sockfd, cmd.c_str(), cmd.size()) < 0)
    {
        m_bStop = true;
        CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket, LCDd not running?", __FUNCTION__);
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
