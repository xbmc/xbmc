/*
*      Copyright (C) 2007-2018 Team XBMC
 *      http://kodi.tv
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

#include "LIRC.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include <lirc/lirc_client.h>
#include <fcntl.h>
#include <sys/socket.h>

CLirc::CLirc() : CThread("Lirc")
{
}

CLirc::~CLirc()
{
  {
    CSingleLock lock(m_critSection);
    if (m_fd > 0)
      shutdown(m_fd, SHUT_RDWR);
  }
  StopThread();
}

void CLirc::Start()
{
  Create();
  SetPriority(GetMinPriority());
}

void CLirc::Process()
{
  m_profileId = CServiceBroker::GetProfileManager().GetCurrentProfileId();
  m_irTranslator.Load("Lircmap.xml");

  while (!m_bStop)
  {
    {
      CSingleLock lock(m_critSection);
      m_fd = lirc_init(const_cast<char*>("kodi"), 0);
      if (m_fd <= 0)
      {
        CSingleExit lock(m_critSection);
        Sleep(1000);
        continue;
      }
    }

    char *code;
    while (!m_bStop)
    {
      int ret = lirc_nextcode(&code);
      if (ret < 0)
      {
        lirc_deinit();
        Sleep(1000);
        break;
      }
      if (code != nullptr)
      {
        if (m_profileId != CServiceBroker::GetProfileManager().GetCurrentProfileId())
        {
          m_profileId = CServiceBroker::GetProfileManager().GetCurrentProfileId();
          m_irTranslator.Load("Lircmap.xml");
        }
        ProcessCode(code);
        free(code);
      }
    }
  }

  lirc_deinit();
}

void CLirc::ProcessCode(char *buf)
{
  // Parse the result. Sample line:
  // 000000037ff07bdd 00 OK mceusb
  char scanCode[128];
  char buttonName[128];
  char repeatStr[4];
  char deviceName[128];
  sscanf(buf, "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

  // Some template LIRC configuration have button names in apostrophes or quotes.
  // If we got a quoted button name, strip 'em
  unsigned int buttonNameLen = strlen(buttonName);
  if (buttonNameLen > 2 &&
      ((buttonName[0] == '\'' && buttonName[buttonNameLen-1] == '\'') ||
      ((buttonName[0] == '"' && buttonName[buttonNameLen-1] == '"'))))
  {
    memmove(buttonName, buttonName + 1, buttonNameLen - 2);
    buttonName[buttonNameLen - 2] = '\0';
  }

  int button = m_irTranslator.TranslateButton(deviceName, buttonName);

  char *end = nullptr;
  long repeat = strtol(repeatStr, &end, 16);
  if (!end || *end != 0)
    CLog::Log(LOGERROR, "LIRC: invalid non-numeric character in expression %s", repeatStr);

  if (repeat == 0)
  {
    CLog::Log(LOGDEBUG, "LIRC: - NEW %s %s %s %s (%s)", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0], buttonName);
    m_firstClickTime = XbmcThreads::SystemClockMillis();

    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = 0;
    g_application.OnEvent(newEvent);
    return;
  }
  else if (repeat > g_advancedSettings.m_remoteDelay)
  {
    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = XbmcThreads::SystemClockMillis() - m_firstClickTime;
    g_application.OnEvent(newEvent);
  }
}
