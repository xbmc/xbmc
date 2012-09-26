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

#include "threads/SystemClock.h"
#include "PlatformInclude.h"
#include "XLCDproc.h"
#include "../utils/log.h"
#include "../utils/TimeUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "XLCDproc_imon.h"
#include "XLCDproc_mdm166a.h"

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
  m_lcdprocIconDevice = NULL;
  m_iCharsetTab = LCD_CHARSET_TAB_HD44780;
}

XLCDproc::~XLCDproc()
{
  if(m_lcdprocIconDevice != NULL)
  {
    delete m_lcdprocIconDevice;
    m_lcdprocIconDevice = NULL;
  }
}

void XLCDproc::ReadAndFlushSocket()
{
  char recvtmp[1024];

  if (read(m_sockfd, recvtmp, 1024) < 0)
  {
    // only spam xbmc.log when something serious happened,
    // EAGAIN literally means "nothing to read", this is fine for us.
    if(errno != EAGAIN)
      CLog::Log(LOGERROR, "XLCDproc::ReadAndFlushSocket - Cannot read/clear response");
  }
}

bool XLCDproc::SendLCDd(const CStdString &command)
{
  if (m_sockfd == -1)
    return false;

  if (write(m_sockfd, command.c_str(), command.size()) < 0)
  {
    CLog::Log(LOGERROR, "XLCDproc::SendLCDd - Cannot send command '%s'.",
      command.c_str());
    return false;
  }

  ReadAndFlushSocket();

  return true;
}

void XLCDproc::Initialize()
{
  int sockfdopt;

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

    if ((sockfdopt = fcntl(m_sockfd, F_GETFL)) == -1)
    {
      CLog::Log(LOGERROR,
        "XLCDproc::%s - Cannot read socket flags, stopping LCD",
          __FUNCTION__);

      CloseSocket();
      m_bStop = true;
    }
    else
    {
      if (fcntl(m_sockfd, F_SETFL, sockfdopt | O_NONBLOCK) == -1)
      {
        CLog::Log(LOGERROR,
          "XLCDproc::%s - Cannot set socket to nonblocking mode, stopping LCD",
            __FUNCTION__);

        CloseSocket();
        m_bStop = true;
      }
    }
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
  char reply[1024] = {};
  if (read(m_sockfd,reply,sizeof(reply) - 1) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket", __FUNCTION__);
    return false;
  }

  unsigned int i=0;
  while ((strncmp("lcd",reply + i,3) != 0 ) && (i < (strlen(reply) - 5))) i++;
  if(sscanf(reply+i,"lcd wid %u hgt %u", &m_iColumns, &m_iRows))
    CLog::Log(LOGDEBUG, "XLCDproc::%s - LCDproc data: Columns %i - Rows %i.", __FUNCTION__, m_iColumns, m_iRows);

  // Check for supported extra icon LCD driver
  RecognizeAndSetIconDriver();

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
  if (!SendLCDd(cmd))
  {
    return false;
  }

  return true;
}

void XLCDproc::RecognizeAndSetIconDriver()
{
  // Get information about the driver
  CStdString info;
  info = "info\n";

  if (write(m_sockfd,info.c_str(),info.size()) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
    return;
  }

  // Receive LCDproc data to determine the driver
  char reply[1024] = {};

  // Receive server's reply
  if (read(m_sockfd, reply, 1024) < 0)
  {
    // Don't go any further if an error occured
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket", __FUNCTION__);
    return;
  }

  // See if running LCDproc driver gave any useful reply.
  // From lcdprocsrc/server/drivers.c on the info command:
  // * Get information from loaded drivers.
  // * \return  Pointer to information string of first driver with get_info() function defined,
  // *          or the empty string if no driver has a get_info() function.
  // "empty string" means "\n", the popular HD44780 is such a candidate...
  if (strncmp(reply, "\n", 1) == 0)
  {
    // Empty (unusable) info reply, WARN that
    CLog::Log(LOGWARNING, "XLCDproc::%s - No usable reply on 'info' command, no icon support!",
      __FUNCTION__);
    return;
  }
  else
  {
    // Log LCDproc 'info' reply
    CLog::Log(LOGNOTICE, "XLCDproc::%s - Plain driver name is: %s", __FUNCTION__, reply);
  }

  // to support older and newer versions of the lcdproc driver for the imon-lcd:
  CStdString driverStringImonLcd = "SoundGraph iMON";
  CStdString driverStringImonLcd2 = "LCD";

  CStdString driverStringMdm166a = "mdm166a";

  if ((strstr(reply, driverStringImonLcd.c_str()) != NULL) && (strstr(reply,
      driverStringImonLcd2.c_str()) != NULL))
  {
    CLog::Log(LOGINFO, "XLCDproc::%s - Driver is: %s", __FUNCTION__,
        "SoundGraph iMON LCD driver");
    m_lcdprocIconDevice = new XLCDproc_imon(m_sockfd);
    m_iCharsetTab = LCD_CHARSET_TAB_IMONMDM;
  }
  else if (strstr(reply, driverStringMdm166a.c_str()) != NULL)
  {
    CLog::Log(LOGINFO, "XLCDproc::%s - Driver is: %s", __FUNCTION__,
              "Targa USB Graphic Vacuum Fluorescent Display (mdm166a)");
    m_lcdprocIconDevice = new XLCDproc_mdm166a(m_sockfd);
    m_iCharsetTab = LCD_CHARSET_TAB_IMONMDM;
  }
}

void XLCDproc::CloseSocket()
{
  if(m_lcdprocIconDevice != NULL)
  {
    delete m_lcdprocIconDevice;
    m_lcdprocIconDevice = NULL;
  }

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

  if (!SendLCDd(cmd))
  {
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
    
    for(int i=0; i<4; i++)
      m_strLine[i] = "";
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
  if (!SendLCDd(cmd))
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
  if (m_lcdprocIconDevice != NULL)
  {
    m_lcdprocIconDevice->HandleStop();
    ReadAndFlushSocket();
  }

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
  if (!SendLCDd(cmd))
  {
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
  if (!SendLCDd(cmd))
  {
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
  StringToLCDCharSet(strLineLong, m_iCharsetTab);

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

    if (!SendLCDd(cmd))
    {
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

bool XLCDproc::SendIconStatesToDisplay()
{
  if (m_lcdprocIconDevice != NULL)
  {
    if (m_lcdprocIconDevice->SendIconStatesToDisplay() == false)
    {
      CLog::Log(LOGERROR, "XLCDproc::%s - Icon state update failed, unable to write to socket", __FUNCTION__);
      CloseSocket();
      return false;
    }

    ReadAndFlushSocket();
  }

  return true;
}

void XLCDproc::HandleStop(void)
{
  if (m_lcdprocIconDevice != NULL)
  {
    m_lcdprocIconDevice->HandleStop();
    ReadAndFlushSocket();
  }
}

void XLCDproc::SetIconMovie(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconMovie(on);
}

void XLCDproc::SetIconMusic(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconMusic(on);
}

void XLCDproc::SetIconWeather(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconWeather(on);
}

void XLCDproc::SetIconTV(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconTV(on);
}

void XLCDproc::SetIconPhoto(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconPhoto(on);
}

void XLCDproc::SetIconResolution(LCD_RESOLUTION_INDICATOR resolution)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconResolution(resolution);
}

void XLCDproc::SetProgressBar1(double progress)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar1(progress);
}

void XLCDproc::SetProgressBar2(double progress)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar2(progress);
}

void XLCDproc::SetProgressBar3(double progress)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar3(progress);
}

void XLCDproc::SetProgressBar4(double progress)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar4(progress);
}

void XLCDproc::SetIconMute(bool on)
{
  if (m_lcdprocIconDevice != NULL)
      m_lcdprocIconDevice->SetIconMute(on);
}

void XLCDproc::SetIconPlaying(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconPlaying(on);
}

void XLCDproc::SetIconPause(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconPause(on);
}

void XLCDproc::SetIconRepeat(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconRepeat(on);
}

void XLCDproc::SetIconShuffle(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconShuffle(on);
}

void XLCDproc::SetIconAlarm(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconAlarm(on);
}

void XLCDproc::SetIconRecord(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconRecord(on);
}

void XLCDproc::SetIconVolume(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconVolume(on);
}

void XLCDproc::SetIconTime(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconTime(on);
}

void XLCDproc::SetIconSPDIF(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconSPDIF(on);
}

void XLCDproc::SetIconDiscIn(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconDiscIn(on);
}

void XLCDproc::SetIconSource(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconSource(on);
}

void XLCDproc::SetIconFit(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconFit(on);
}

void XLCDproc::SetIconSCR1(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconSCR1(on);
}

void XLCDproc::SetIconSCR2(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconSCR2(on);
}

// codec icons - video: video stream format ###################################
void XLCDproc::SetIconMPEG(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconMPEG(on);
}

void XLCDproc::SetIconDIVX(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconDIVX(on);
}

void XLCDproc::SetIconXVID(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconXVID(on);
}

void XLCDproc::SetIconWMV(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconWMV(on);
}

// codec icons - video: audio stream format #################################
void XLCDproc::SetIconMPGA(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconMPGA(on);
}

void XLCDproc::SetIconAC3(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconAC3(on);
}

void XLCDproc::SetIconDTS(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconDTS(on);
}

void XLCDproc::SetIconVWMA(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconVWMA(on);
}
// codec icons - audio format ###############################################
void XLCDproc::SetIconMP3(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconMP3(on);
}

void XLCDproc::SetIconOGG(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconOGG(on);
}

void XLCDproc::SetIconAWMA(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconAWMA(on);
}

void XLCDproc::SetIconWAV(bool on)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconWAV(on);
}

void XLCDproc::SetIconAudioChannels(int channels)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconAudioChannels(channels);
}
