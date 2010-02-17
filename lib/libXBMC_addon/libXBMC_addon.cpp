/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "libXBMC_addon.h"
#include "AddonHelpers_local.h"

using namespace std;

AddonCB *m_cb = NULL;

void XBMC_register_me(ADDON_HANDLE hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_addon-ERROR: XBMC_register_me is called with NULL handle !!!\n");
  else
    m_cb = (AddonCB*) hdl;
  return;
}

bool XBMC_get_setting(string settingName, void *settingValue)
{
  if (m_cb == NULL)
    return false;

  return m_cb->AddOn.GetSetting(m_cb->addonData, settingName.c_str(), settingValue);
}

void XBMC_log(const addon_log_t loglevel, const char *format, ... )
{
  if (m_cb == NULL)
    return;

  char buffer[16384];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  m_cb->AddOn.Log(m_cb->addonData, loglevel, buffer);
}

void XBMC_queue_notification(const queue_msg_t type, const char *format, ... )
{
  if (m_cb == NULL)
    return;

  char buffer[16384];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  m_cb->AddOn.QueueNotification(m_cb->addonData, type, buffer);
}

void XBMC_unknown_to_utf8(string &str)
{
  if (m_cb == NULL)
    return;

  string buffer = m_cb->Utils.UnknownToUTF8(str.c_str());
  str = buffer;
}
