/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralNyxboard.h"

#include "Application.h"
#include "PeripheralHID.h"
#include "utils/log.h"

using namespace PERIPHERALS;

CPeripheralNyxboard::CPeripheralNyxboard(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheralHID(manager, scanResult, bus)
{
  m_features.push_back(FEATURE_NYXBOARD);
}

bool CPeripheralNyxboard::LookupSymAndUnicode(XBMC_keysym &keysym, uint8_t *key, char *unicode)
{
  std::string strCommand;
  if (keysym.sym == XBMCK_F7 && keysym.mod == XBMCKMOD_NONE && GetSettingBool("enable_flip_commands"))
  {
    /* switched to keyboard side */
    CLog::Log(LOGDEBUG, "%s - switched to keyboard side", __FUNCTION__);
    strCommand = GetSettingString("flip_keyboard");
  }
  else if (keysym.sym == XBMCK_F7 && keysym.mod == XBMCKMOD_LCTRL && GetSettingBool("enable_flip_commands"))
  {
    /* switched to remote side */
    CLog::Log(LOGDEBUG, "%s - switched to remote side", __FUNCTION__);
    strCommand = GetSettingString("flip_remote");
  }

  if (!strCommand.empty())
  {
    CLog::Log(LOGDEBUG, "%s - executing command '%s'", __FUNCTION__, strCommand.c_str());
    if (g_application.ExecuteXBMCAction(strCommand))
    {
      *key = 0;
      *unicode = (char) 0;
      return true;
    }
  }

  return false;
}
