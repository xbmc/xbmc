/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "General.h"

#include "addons/kodi-addon-dev-kit/include/kodi/General.h"

#include "Application.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/LangCodeExpander.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"

using namespace kodi; // addon-dev-kit namespace

namespace ADDON
{

void Interface_General::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi = static_cast<AddonToKodiFuncTable_kodi*>(malloc(sizeof(AddonToKodiFuncTable_kodi)));

  addonInterface->toKodi->kodi->open_settings_dialog = open_settings_dialog;
  addonInterface->toKodi->kodi->get_localized_string = get_localized_string;
  addonInterface->toKodi->kodi->unknown_to_utf8 = unknown_to_utf8;
  addonInterface->toKodi->kodi->get_language = get_language;
  addonInterface->toKodi->kodi->queue_notification = queue_notification;
  addonInterface->toKodi->kodi->get_md5 = get_md5;
}

void Interface_General::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi && /* <-- needed as long as the old addon way is used */
      addonInterface->toKodi->kodi)
  {
    free(addonInterface->toKodi->kodi);
    addonInterface->toKodi->kodi = nullptr;
  }
}

bool Interface_General::open_settings_dialog(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p')", __FUNCTION__, addon);
    return false;
  }

  // show settings dialog
  AddonPtr addonInfo;
  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), addonInfo))
  {
    CLog::Log(LOGERROR, "Interface_General::%s - Could not get addon information for '%s'", __FUNCTION__, addon->ID().c_str());
    return false;
  }

  return CGUIDialogAddonSettings::ShowForAddon(addonInfo);
}

char* Interface_General::get_localized_string(void* kodiBase, long dwCode)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p')", __FUNCTION__, addon);
    return nullptr;
  }

  if (g_application.m_bStop)
    return nullptr;

  std::string string;
  if ((dwCode >= 30000 && dwCode <= 30999) || (dwCode >= 32000 && dwCode <= 32999))
    string = g_localizeStrings.GetAddonString(addon->ID(), dwCode).c_str();
  else
    string = g_localizeStrings.Get(dwCode).c_str();

  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_General::unknown_to_utf8(void* kodiBase, const char* source, bool* ret, bool failOnBadChar)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !source || !ret)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p', source='%p', ret='%p')", __FUNCTION__, addon, source, ret);
    return nullptr;
  }
   
  std::string string;
  *ret = g_charsetConverter.unknownToUTF8(source, string, failOnBadChar);
  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_General::get_language(void* kodiBase, int format, bool region)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p')", __FUNCTION__, addon);
    return nullptr;
  }

  std::string string = g_langInfo.GetEnglishLanguageName();
  switch (format)
  {
    case LANG_FMT_ISO_639_1:
    {
      std::string langCode;
      g_LangCodeExpander.ConvertToISO6391(string, langCode);
      string = langCode;
      if (region)
      {
        std::string region2Code;
        g_LangCodeExpander.ConvertToISO6391(g_langInfo.GetRegionLocale(), region2Code);
        if (!region2Code.empty())
          string += "-" + region2Code;
      }
      break;
    }
    case LANG_FMT_ISO_639_2:
    {
      std::string langCode;
      g_LangCodeExpander.ConvertToISO6392B(string, langCode);
      string = langCode;
      if (region)
      {
        std::string region3Code;
        g_LangCodeExpander.ConvertToISO6392B(g_langInfo.GetRegionLocale(), region3Code);
        if (!region3Code.empty())
          string += "-" + region3Code;
      }
      break;
    }
    case LANG_FMT_ENGLISH_NAME:
    default:
    {
      if (region)
        string += "-" + g_langInfo.GetCurrentRegion();
      break;
    }
  }

  char* buffer = strdup(string.c_str());
  return buffer;
}

bool Interface_General::queue_notification(void* kodiBase, int type, const char* header,
                                           const char* message, const char* imageFile,
                                           unsigned int displayTime, bool withSound,
                                           unsigned int messageTime)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || message == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p', message='%p')", __FUNCTION__, addon, message);
    return false;
  }

  std::string usedHeader;
  if (header && strlen(header) > 0)
    usedHeader = header;
  else
    usedHeader = addon->Name();

  QueueMsg qtype = static_cast<QueueMsg>(type);

  if (qtype != QUEUE_OWN_STYLE)
  {
    CGUIDialogKaiToast::eMessageType usedType;
    switch (qtype)
    {
    case QUEUE_WARNING:
      usedType = CGUIDialogKaiToast::Warning;
      withSound = true;
      CLog::Log(LOGDEBUG, "Interface_General::%s - %s - Warning Message: '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
    case QUEUE_ERROR:
      usedType = CGUIDialogKaiToast::Error;
      withSound = true;
      CLog::Log(LOGDEBUG, "Interface_General::%s - %s - Error Message : '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
    case QUEUE_INFO:
    default:
      usedType = CGUIDialogKaiToast::Info;
      withSound = false;
      CLog::Log(LOGDEBUG, "Interface_General::%s - %s - Info Message : '%s'", __FUNCTION__, addon->Name().c_str(), message);
      break;
    }

    if (imageFile && strlen(imageFile) > 0)
    {
      CLog::Log(LOGERROR, "Interface_General::%s - To use given image file '%s' must be type value set to 'QUEUE_OWN_STYLE'", __FUNCTION__, imageFile);
    }

    CGUIDialogKaiToast::QueueNotification(usedType, usedHeader, message, 3000, withSound);
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(imageFile, usedHeader, message, displayTime, withSound, messageTime);
  }
  return true;
}

void Interface_General::get_md5(void* kodiBase, const char* text, char* md5)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || text == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::%s - invalid data (addon='%p', text='%p')", __FUNCTION__, addon, text);
    return;
  }

  std::string md5Int = XBMC::XBMC_MD5::GetMD5(std::string(text));
  strncpy(md5, md5Int.c_str(), 40);
}

} /* namespace ADDON */
