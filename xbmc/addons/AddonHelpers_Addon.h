#pragma once
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

#include "AddonHelpers_local.h"

namespace ADDON
{

class CAddonHelpers_Addon
{
public:
  CAddonHelpers_Addon(CAddon* addon);
  ~CAddonHelpers_Addon();

  /**! \name General Functions */
  CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  /**! \name Callback functions */
  static void AddOnLog(void *addonData, const addon_log_t loglevel, const char *msg);
  static void QueueNotification(void *addonData, const queue_msg_t type, const char *msg);
  static bool GetAddonSetting(void *addonData, const char* settingName, void *settingValue);
  static char* UnknownToUTF8(const char *sourceDest);
  static const char* GetLocalizedString(const void* addonData, long dwCode);
  static const char* GetDVDMenuLanguage(const void* addonData);

private:
  CB_AddOnLib  *m_callbacks;
  CAddon       *m_addon;
};

}; /* namespace ADDON */
