#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

class CAddonCallbacksAddon
{
public:
  CAddonCallbacksAddon(CAddon* addon);
  ~CAddonCallbacksAddon();

  /*!
   * @return The callback table.
   */
  CB_AddOnLib *GetCallbacks() { return m_callbacks; }

  /*!
   * @brief Add a message to XBMC's log.
   * @param addonData A pointer to the add-on.
   * @param addonLogLevel The log level of the message.
   * @param strMessage The message itself.
   */
  static void AddOnLog(void *addonData, const addon_log_t addonLogLevel, const char *strMessage);

  /*!
   * @brief Queue a notification in the GUI.
   * @param addonData A pointer to the add-on.
   * @param type The message type.
   * @param strMessage The message to display.
   */
  static void QueueNotification(void *addonData, const queue_msg_t type, const char *strMessage);

  /*!
   * @brief Get a settings value for this add-on.
   * @param addonData A pointer to the add-on.
   * @param settingName The name of the setting to get.
   * @param settingValue The value.
   * @return True if the settings was fetched successfully, false otherwise.
   */
  static bool GetAddonSetting(void *addonData, const char *strSettingName, void *settingValue);

  /*!
   * @brief Translate a string with an unknown encoding to UTF8.
   * @param sourceDest The source string.
   * @return The converted string.
   */
  static char *UnknownToUTF8(const char *strSource);

  /*!
   * @brief Get a localised message.
   * @param addonData A pointer to the add-on.
   * @param dwCode The code of the message to get.
   * @return The message.
   */
  static const char *GetLocalizedString(const void* addonData, long dwCode);

  /*!
   * @brief Get the DVD menu language.
   * @param addonData A pointer to the add-on.
   * @return The language.
   */
  static const char *GetDVDMenuLanguage(const void* addonData);

private:
  CB_AddOnLib  *m_callbacks; /*!< callback addresses */
  CAddon       *m_addon;     /*!< the add-on */
};

}; /* namespace ADDON */
