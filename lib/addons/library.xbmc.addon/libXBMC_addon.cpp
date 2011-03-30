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
#include "../../../addons/library.xbmc.addon/libXBMC_addon.h"
#include "../../../xbmc/addons/AddonCallbacks.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


using namespace std;

AddonCB *m_Handle = NULL;
CB_AddOnLib *m_cb = NULL;

extern "C"
{

DLLEXPORT int XBMC_register_me(void *hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_addon-ERROR: XBMC_register_me is called with NULL handle !!!\n");
  else
  {
    m_Handle = (AddonCB*) hdl;
    m_cb     = m_Handle->AddOnLib_RegisterMe(m_Handle->addonData);
    if (!m_cb)
      fprintf(stderr, "libXBMC_addon-ERROR: XBMC_register_me can't get callback table from XBMC !!!\n");
    else
      return 1;
  }
  return 0;
}

DLLEXPORT void XBMC_unregister_me()
{
  if (m_Handle && m_cb)
    m_Handle->AddOnLib_UnRegisterMe(m_Handle->addonData, m_cb);
}

DLLEXPORT void XBMC_log(const addon_log_t loglevel, const char *format, ... )
{
  if (m_cb == NULL)
    return;

  char buffer[16384];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  m_cb->Log(m_Handle->addonData, loglevel, buffer);
}

DLLEXPORT bool XBMC_get_setting(const char* settingName, void *settingValue)
{
  if (m_cb == NULL)
    return false;

  return m_cb->GetSetting(m_Handle->addonData, settingName, settingValue);
}

DLLEXPORT void XBMC_queue_notification(const queue_msg_t type, const char *format, ... )
{
  if (m_cb == NULL)
    return;

  char buffer[16384];
  va_list args;
  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  m_cb->QueueNotification(m_Handle->addonData, type, buffer);
}

DLLEXPORT void XBMC_unknown_to_utf8(string &str)
{
  if (m_cb == NULL)
    return;

  string buffer = m_cb->UnknownToUTF8(str.c_str());
  str = buffer;
}

DLLEXPORT const char* XBMC_get_localized_string(int dwCode)
{
  if (m_cb == NULL)
    return "";

  string buffer = m_cb->GetLocalizedString(m_Handle->addonData, dwCode);
  return buffer.c_str();
}

DLLEXPORT const char* XBMC_get_dvd_menu_language()
{
  if (m_cb == NULL)
    return "";

  string buffer = m_cb->GetDVDMenuLanguage(m_Handle->addonData);
  return buffer.c_str();
}

};
