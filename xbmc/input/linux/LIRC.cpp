/*
*      Copyright (C) 2007-2012 Team XBMC
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

#if defined (HAS_LIRC)

#include "threads/SystemClock.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>
#include <unistd.h>
#include "LIRC.h"
#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#endif
#include "input/ButtonTranslator.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "utils/TimeUtils.h"

CRemoteControl g_RemoteControl;

CRemoteControl::CRemoteControl()
{
  m_fd = -1;
  m_file = NULL;
  m_bInitialized = false;
  m_button = 0;
  m_holdTime = 0;
  m_used = true;
  m_deviceName = LIRC_DEVICE;
  m_inotify_fd = -1;
  m_inotify_wd = -1;
  m_bLogConnectFailure = true;
  m_lastInitAttempt = -5000;
  m_initRetryPeriod = 5000;
  m_inReply = false;
  m_nrSending = 0;
  Reset();
}

CRemoteControl::~CRemoteControl()
{
  if (m_file != NULL)
    fclose(m_file);
}

void CRemoteControl::setUsed(bool value)
{
  m_used=value;
  if (!value)
    CLog::Log(LOGINFO, "LIRC %s: disabled", __FUNCTION__);
  else
  {
    m_lastInitAttempt = -5000;
    m_initRetryPeriod = 5000;
  }
}

void CRemoteControl::Reset()
{
  m_button = 0;
  m_holdTime = 0;
}

void CRemoteControl::Disconnect()
{
  if (!m_used)
    return;

  if (m_fd != -1) 
  {
    m_bInitialized = false;
    if (m_file != NULL)
      fclose(m_file);
    if (m_fd != -1)
      close(m_fd);
    m_fd = -1;
    m_file = NULL;
#ifdef HAVE_INOTIFY
    if (m_inotify_wd >= 0) {
      inotify_rm_watch(m_inotify_fd, m_inotify_wd);
      m_inotify_wd = -1;
    }
    if (m_inotify_fd >= 0)
      close(m_inotify_fd);
#endif

    m_inReply = false;
    m_nrSending = 0;
    m_sendData.clear();
  }
}

void CRemoteControl::setDeviceName(const CStdString& value)
{
  if (value.length()>0)
    m_deviceName=value;
  else
    m_deviceName=LIRC_DEVICE;
  if (m_bInitialized)
  {
    Disconnect();
    Initialize();
  }
}

void CRemoteControl::Initialize()
{
  struct sockaddr_un addr;
  unsigned int now = XbmcThreads::SystemClockMillis();

  if (!m_used || (now - m_lastInitAttempt) < (unsigned int)m_initRetryPeriod)
    return;
  
  m_lastInitAttempt = now;
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, m_deviceName.c_str());

  CLog::Log(LOGINFO, "LIRC %s: using: %s", __FUNCTION__, addr.sun_path);

  // Open the socket from which we will receive the remote commands
  if ((m_fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
  {
    // Connect to the socket
    if (connect(m_fd, (struct sockaddr *)&addr, sizeof(addr)) != -1)
    {
      int opts;
      m_bLogConnectFailure = true;
      if ((opts = fcntl(m_fd,F_GETFL)) != -1)
      {
        // Set the socket to non-blocking
        opts = (opts | O_NONBLOCK);
        if (fcntl(m_fd,F_SETFL,opts) != -1)
        {
          if ((m_file = fdopen(m_fd, "r+")) != NULL)
          {
#ifdef HAVE_INOTIFY
            // Setup inotify so we can disconnect if lircd is restarted
            if ((m_inotify_fd = inotify_init()) >= 0)
            {
              // Set the fd non-blocking
              if ((opts = fcntl(m_inotify_fd, F_GETFL)) != -1)
              {
                opts |= O_NONBLOCK;
                if (fcntl(m_inotify_fd, F_SETFL, opts) != -1)
                {
                  // Set an inotify watch on the lirc device
                  if ((m_inotify_wd = inotify_add_watch(m_inotify_fd, m_deviceName.c_str(), IN_DELETE_SELF)) != -1)
                  {
                    m_bInitialized = true;
                    CLog::Log(LOGINFO, "LIRC %s: sucessfully started", __FUNCTION__);
                  }
                  else
                    CLog::Log(LOGDEBUG, "LIRC: Failed to initialize Inotify. LIRC device will not be monitored.");
                }
              }
            }
#else
            m_bInitialized = true;
            CLog::Log(LOGINFO, "LIRC %s: sucessfully started", __FUNCTION__);
#endif
          }
          else
            CLog::Log(LOGERROR, "LIRC %s: fdopen failed: %s", __FUNCTION__, strerror(errno));
        }
        else
          CLog::Log(LOGERROR, "LIRC %s: fcntl(F_SETFL) failed: %s", __FUNCTION__, strerror(errno));
      }
      else
        CLog::Log(LOGERROR, "LIRC %s: fcntl(F_GETFL) failed: %s", __FUNCTION__, strerror(errno));
    }
    else
    {
      if (m_bLogConnectFailure)
      {
        CLog::Log(LOGINFO, "LIRC %s: connect failed: %s", __FUNCTION__, strerror(errno));
        m_bLogConnectFailure = false;
      }
    }
  }
  else
    CLog::Log(LOGINFO, "LIRC %s: socket failed: %s", __FUNCTION__, strerror(errno));
  if (!m_bInitialized)
  {
    Disconnect();
    m_initRetryPeriod *= 2;
    if (m_initRetryPeriod > 60000)
    {
      m_used = false;
      CLog::Log(LOGDEBUG, "Failed to connect to LIRC. Giving up.");
    }
    else
      CLog::Log(LOGDEBUG, "Failed to connect to LIRC. Retry in %ds.", m_initRetryPeriod/1000);
  }
  else
    m_initRetryPeriod = 5000;
}

bool CRemoteControl::CheckDevice() {
#ifdef HAVE_INOTIFY
  if (m_inotify_fd < 0 || m_inotify_wd < 0)
    return true; // inotify wasn't setup for some reason, assume all is well
  int bufsize = sizeof(struct inotify_event) + PATH_MAX;
  char buf[bufsize];
  int ret = read(m_inotify_fd, buf, bufsize);
  for (int i = 0; i + (int)sizeof(struct inotify_event) <= ret;) {
    struct inotify_event* e = (struct inotify_event*)(buf+i);
    if (e->mask & IN_DELETE_SELF) {
      CLog::Log(LOGDEBUG, "LIRC device removed, disconnecting...");
      Disconnect();
      return false;
    }
    i += sizeof(struct inotify_event)+e->len;
  }
#endif
  return true;
}

void CRemoteControl::Update()
{
  if (!m_bInitialized || !m_used )
    return;

  if (!CheckDevice())
    return;

  uint32_t now = XbmcThreads::SystemClockMillis();

  // Read a line from the socket
  while (fgets(m_buf, sizeof(m_buf), m_file) != NULL)
  {
    // Remove the \n
    m_buf[strlen(m_buf)-1] = '\0';

    // Parse the result. Sample line:
    // 000000037ff07bdd 00 OK mceusb
    char scanCode[128];
    char buttonName[128];
    char repeatStr[4];
    char deviceName[128];
    sscanf(m_buf, "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

    //beginning of lirc reply packet
    //we get one when lirc is done sending something
    if (!m_inReply && strcmp("BEGIN", scanCode) == 0)
    {
      m_inReply = true;
      continue;
    }
    else if (m_inReply && strcmp("END", scanCode) == 0) //end of lirc reply packet
    {
      m_inReply = false;
      if (m_nrSending > 0)
        m_nrSending--;
      continue;
    }

    if (m_inReply)
      continue;

    // Some template LIRC configuration have button names in apostrophes or quotes.
    // If we got a quoted button name, strip 'em
    unsigned int buttonNameLen = strlen(buttonName);
    if ( buttonNameLen > 2
    && ( (buttonName[0] == '\'' && buttonName[buttonNameLen-1] == '\'')
         || ((buttonName[0] == '"' && buttonName[buttonNameLen-1] == '"') ) ) )
    {
      memmove( buttonName, buttonName + 1, buttonNameLen - 2 );
      buttonName[ buttonNameLen - 2 ] = '\0';
    }

    m_button = CButtonTranslator::GetInstance().TranslateLircRemoteString(deviceName, buttonName);

    char *end = NULL;
    long repeat = strtol(repeatStr, &end, 16);
    if (!end || *end != 0)
      CLog::Log(LOGERROR, "LIRC: invalid non-numeric character in expression %s", repeatStr);
    if (repeat == 0)
    {
      CLog::Log(LOGDEBUG, "LIRC: %s - NEW at %d:%s (%s)", __FUNCTION__, now, m_buf, buttonName);
      m_firstClickTime = now;
      m_holdTime = 0;
      return;
    }
    else if (repeat > g_advancedSettings.m_remoteDelay)
    {
      m_holdTime = now - m_firstClickTime;
    }
    else
    {
      m_holdTime = 0;
      m_button = 0;
    }
  }

  //drop commands when already sending
  //because keypresses come in faster than lirc can send we risk hammering the daemon with commands
  if (m_nrSending > 0)
  {
    m_sendData.clear();
  }
  else if (!m_sendData.empty())
  {
    fputs(m_sendData.c_str(), m_file);
    fflush(m_file);

    //nr of newlines equals nr of commands
    for (int i = 0; i < (int)m_sendData.size(); i++)
      if (m_sendData[i] == '\n')
        m_nrSending++;

    m_sendData.clear();
  }

  if (feof(m_file) != 0)
    Disconnect();
}

WORD CRemoteControl::GetButton()
{
  return m_button;
}

unsigned int CRemoteControl::GetHoldTime() const
{
  return m_holdTime;
}

void CRemoteControl::AddSendCommand(const CStdString& command)
{
  if (!m_bInitialized || !m_used)
    return;

  m_sendData += command;
  m_sendData += '\n';
}

#endif
