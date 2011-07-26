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
#include "XLCDProc_imon.h"
#include "XLCDProc_mdm166a.h"
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SCROLL_SPEED_IN_MSEC 250

// initialize class variables
int XLCDproc::outputValue = 0;
int XLCDproc::outputValueProgressBars = 0;
int XLCDproc::outputValueOld = 1; // for correct icon-initialization this needs to be different to the actual value
int XLCDproc::outputValueProgressBarsOld = 1; // for correct icon-initialization this needs to be different to the actual value
int XLCDproc::m_sockfd = -1;
int XLCDproc::saChanger = 0;

XLCDproc::XLCDproc()
{
  m_iActualpos   = 0;
  m_iBackLight   = 32;
  m_iLCDContrast = 50;
  m_bStop        = true;
//  m_sockfd       = -1;
  m_lastInitAttempt = 0;
  m_initRetryInterval = INIT_RETRY_INTERVAL;
  m_used = true;
  m_lcdprocIconDevice = NULL;
  scale = 1.0 / log(256.0);
  audioDataBeforeDisplaying = 1;
}

XLCDproc::~XLCDproc()
{
  delete m_lcdprocIconDevice;
}

void XLCDproc::Initialize()
{
  if (!m_used || !g_guiSettings.GetBool("videoscreen.haslcd"))
    return;//nothing to do

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

    RecognizeAndSetDriver();
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

  usleep(msWaitTime); // wait for the answer

  // Receive LCDproc data to determine row and column information
  char reply[1024];
  if (read(m_sockfd,reply,1024) == -1)
  {
    CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket", __FUNCTION__);
    return false;
  }

  unsigned int i=0;
  while ((strncmp("lcd",reply + i,3) != 0 ) && (i < (strlen(reply) - 5))) i++;
//  if(sscanf(reply+i,"lcd wid %u hgt %u", &m_iColumns, &m_iRows))
//    CLog::Log(LOGDEBUG, "XLCDproc::%s - LCDproc data: Columns %i - Rows %i.", __FUNCTION__, m_iColumns, m_iRows);
  if (sscanf(reply + i, "lcd wid %u hgt %u cellwid %u cellhgt %u", &m_iColumns,
        &m_iRows, &m_iCellWidth, &m_iCellHeight))
  {
    CLog::Log(
      LOGDEBUG,
      "XLCDproc::%s - LCDproc data: Columns %i - Rows %i - Cellheight %i - Cellwidth %i.",
      __FUNCTION__, m_iColumns, m_iRows, m_iCellHeight, m_iCellWidth);
    // we found the number of rows and columns - override the advanced settings with it to get the progress bar working correctly
    g_advancedSettings.m_lcdColumns = m_iColumns;
    g_advancedSettings.m_lcdRows = m_iRows;
  }

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

void XLCDproc::RecognizeAndSetDriver()
{
  // Receive LCDproc data to determine the driver
  char reply[1024];

  // Get information about the driver
  CStdString info;
  info = "info\n";

  writeSocketAndLog(info, __FUNCTION__);

  for (unsigned int ms = 0; ms < msTimeout; ms += msWaitTime)
  {
    usleep(msWaitTime); // wait for the answer

    // Receive server's reply
    if (read(m_sockfd, reply, 1024) < 0)
    {
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to read from socket",
          __FUNCTION__);
    }
    else
    {
      CLog::Log(LOGINFO, "XLCDproc::%s - Plain driver name is: %s",
          __FUNCTION__, reply);
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
      m_lcdprocIconDevice = new XLCDProc_imon();
      break;
    } else if (strstr(reply, driverStringMdm166a.c_str()) != NULL)
    {
      CLog::Log(LOGINFO, "XLCDproc::%s - Driver is: %s", __FUNCTION__,
                "Targa USB Graphic Vacuum Fluorescent Display (mdm166a)");
      m_lcdprocIconDevice = new XLCDProc_mdm166a();
      break;
    }
    //    CLog::Log(LOGERROR, "XLCDproc::%s - Driver is: %s", __FUNCTION__, reply);
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
  // clear any icons
  outputValue = 0;
  SendIconStatesToDisplay();
  usleep(10000); // we have to wait a bit until the message has been sent to the display

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
    strLineLong += " * ";

  if (strLineLong != m_strLine[iLine])
  {
    CStdString cmd;
    int ln = iLine + 1;

//    if (g_advancedSettings.m_lcdScrolldelay != 0)
//      cmd.Format("widget_set xbmc line%i 1 %i %i %i m %i \"%s\"\n", ln, ln, m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay, strLineLong.c_str());
//    else
//      cmd.Format("widget_set xbmc line%i 1 %i \"%s\"\n", ln, ln, strLineLong.c_str());
//
//    if (write(m_sockfd, cmd.c_str(), cmd.size()) == -1)
//    {
//      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", __FUNCTION__);
//      CloseSocket();
//      return;
//    }

	if (g_advancedSettings.m_lcdScrolldelay != 0)
	{
	  if (GetIsSpectrumAnalyzerWorking() && ln == 1) // needed in order to allow navigation while playing back music
	  { // unfortunately this is needed. If this is not active, the first line (its content) is missing when playing back a song the very first time
		cmd.Format("widget_set xbmc SaScroller 1 %i %i %i m %i \"%s\"\n", ln,
			m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay,
			strLineLong.c_str());
		//    	  cmd.Format("widget_set xbmc line%i 1 %i %i %i m %i \"%s\"\n", ln, ln, m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay, strLineLong.c_str());
	  }
	  else if (!GetIsSpectrumAnalyzerWorking())
	  {
		cmd.Format("widget_set xbmc line%i 1 %i %i %i m %i \"%s\"\n", ln, ln,
			m_iColumns, ln, g_advancedSettings.m_lcdScrolldelay,
			strLineLong.c_str());
	  }
	}
	else
	  cmd.Format("widget_set xbmc line%i 1 %i \"%s\"\n", ln, ln,
		  strLineLong.c_str());

	if (write(m_sockfd, cmd.c_str(), cmd.size()) < 0)
	{
	  m_bStop = true;
	  CLog::Log(LOGERROR,
		  "XLCDproc::%s - Unable to write to socket, LCDd not running?",
		  __FUNCTION__);
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
  CStdString cmd = "";

  if ((m_lcdprocIconDevice != NULL)
      && (!m_lcdprocIconDevice->SendIconStatesToDisplay()))
  {
    outputValueOld = outputValue;
    cmd.Format("output %d\n", outputValue);

    writeSocketAndLog(cmd, __FUNCTION__);

    return true;
  }
  return false;
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

void XLCDproc::SetIconResolution(LCD_RESOLUTION resolution)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetIconResolution(resolution);
}

void XLCDproc::SetProgressBar1(double progress)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar1(progress);
}

void XLCDproc::SetProgressBar2(double volume)
{
  if (m_lcdprocIconDevice != NULL)
    m_lcdprocIconDevice->SetProgressBar2(volume);
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

bool XLCDproc::writeSocketAndLog(CStdString & cmd, const char *functionName)
{
  // Send to server
  if (!m_bStop)
  {
    if (write(m_sockfd, cmd.c_str(), cmd.size()) < 0)
    {
      CLog::Log(LOGERROR, "XLCDproc::%s - Unable to write to socket", functionName);
      return false;
    }
    return true;
  }
  return false;
}

void XLCDproc::InitializeSpectrumAnalyzer()
{
  CStdString cmd = ""; // for the SpAn-Bars
  unsigned int i = 0;
  CLog::Log(LOGINFO, "XLCDproc::%s!", __FUNCTION__);

  cmd.append("widget_del xbmc line1\n");
  cmd.append("widget_del xbmc line2\n");
  cmd.append("widget_del xbmc line3\n");
  cmd.append("widget_del xbmc line4\n");
  writeSocketAndLog(cmd, __FUNCTION__);

  for (i = 0; i < m_iColumns; i++)
  {
    cmd.Format("widget_add xbmc SaBar%d vbar\n", i + 1);
    writeSocketAndLog(cmd, __FUNCTION__);
  }

  cmd.Format("widget_add xbmc SaScroller scroller\n");
  writeSocketAndLog(cmd, __FUNCTION__);
}

void XLCDproc::RemoveSpectrumAnalyzer()
{
  char reply[1024];
  CStdString cmd; // for the SpAn-Bars
  unsigned int i = 0;

  for (i = 0; i < m_iColumns; i++)
  {
    cmd.Format("widget_del xbmc SaBar%d\n", i + 1);
    writeSocketAndLog(cmd, __FUNCTION__);
    if (!m_bStop)
      read(m_sockfd, reply, 1024);
  }

  cmd.Format("widget_del xbmc SaScroller\n");
  writeSocketAndLog(cmd, __FUNCTION__);

  cmd = "";
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

  writeSocketAndLog(cmd, __FUNCTION__);
}

void XLCDproc::SetSpectrumAnalyzerData()
{
  unsigned int i = 0;
  CStdString cmd = "";
  //  CLog::Log(LOGINFO, "XLCDproc::%s!", __FUNCTION__);

  saChanger = (saChanger + 1) % 10;

  for (i = 0; i < m_iColumns; i++)
  {
    cmd.Format("widget_set xbmc SaBar%d %d %d %d\n", i + 1, i + 1, m_iRows,
        (int) ((m_iCellHeight * (m_iRows / 2)/*1 or 2 lines high*/)
            * /*barHeights[i]*/((heights[i] * 30.0f
                / (float) audioDataBeforeDisplaying))) / 100);

    //      CLog::Log(LOGINFO, "XLCDproc::%s %s f%f", __FUNCTION__, cmd.c_str(), heights[i]);

    writeSocketAndLog(cmd, __FUNCTION__);
  }
  audioDataBeforeDisplaying = 1;
}

void XLCDproc::SetSpectrumAnalyzerAudioData(const short* psAudioData,
    int piAudioDataLength, float *pfFreqData, int piFreqDataLength)
{
  unsigned int i = 0;
  int c = 0, y = 0;
  float val;
  int* xscale;

  // scaling for the spectrum analyzer
  if (m_iColumns == 16)
  {
    // for 16 char-displays
    int scales[] =
    { 0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255 };
    xscale = scales;
  }
  else if (m_iColumns == 20)
  {
    // for 20 char-displays
    int scales[] =
    { 0, 1, 3, 5, 9, 13, 19, 26, 34, 43, 53, 64, 76, 89, 105, 120, 140, 160,
        187, 220, 255 };
    xscale = scales;
  }

  //  CLog::Log(LOGINFO, "XLCDproc::%s", __FUNCTION__);

  for (i = 0; i < m_iColumns; i++)
  {
    for (c = xscale[i], y = 0; c < xscale[i + 1]; c++)
    {
      if (c < piAudioDataLength)
      {
        if (psAudioData[c] > y)
          y = (int) psAudioData[c];
      }
      else
        continue;
    }
    y >>= 7;
    if (y > 0)
      val = (logf(y) * scale);
    else
      val = 0;
    if (audioDataBeforeDisplaying == 1)
      heights[i] = val;
    else
      heights[i] += val;

    //    CLog::Log(LOGINFO, "XLCDproc::%s y=%d f%f, i=%d", __FUNCTION__, y, heights[i], i);
  }
  audioDataBeforeDisplaying++;
}
