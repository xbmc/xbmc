/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "General.h"

#include "Application.h"
#include "CompileInfo.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "addons/kodi-addon-dev-kit/include/kodi/General.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "input/KeyboardLayout.h"
#include "input/KeyboardLayoutManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/Digest.h"
#include "utils/LangCodeExpander.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <string.h>

using namespace kodi; // addon-dev-kit namespace
using KODI::UTILITY::CDigest;

namespace ADDON
{

void Interface_General::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi = static_cast<AddonToKodiFuncTable_kodi*>(malloc(sizeof(AddonToKodiFuncTable_kodi)));

  addonInterface->toKodi->kodi->get_addon_info = get_addon_info;
  addonInterface->toKodi->kodi->open_settings_dialog = open_settings_dialog;
  addonInterface->toKodi->kodi->get_localized_string = get_localized_string;
  addonInterface->toKodi->kodi->unknown_to_utf8 = unknown_to_utf8;
  addonInterface->toKodi->kodi->get_language = get_language;
  addonInterface->toKodi->kodi->queue_notification = queue_notification;
  addonInterface->toKodi->kodi->get_md5 = get_md5;
  addonInterface->toKodi->kodi->get_temp_path = get_temp_path;
  addonInterface->toKodi->kodi->get_region = get_region;
  addonInterface->toKodi->kodi->get_free_mem = get_free_mem;
  addonInterface->toKodi->kodi->get_global_idle_time = get_global_idle_time;
  addonInterface->toKodi->kodi->kodi_version = kodi_version;
  addonInterface->toKodi->kodi->get_current_skin_id = get_current_skin_id;
  addonInterface->toKodi->kodi->get_keyboard_layout = get_keyboard_layout;
  addonInterface->toKodi->kodi->change_keyboard_layout = change_keyboard_layout;
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

char* Interface_General::get_addon_info(void* kodiBase, const char* id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', id='{}')", __FUNCTION__,
              kodiBase, static_cast<const void*>(id));
    return nullptr;
  }

  std::string str;
  if (StringUtils::CompareNoCase(id, "author") == 0)
    str = addon->Author();
  else if (StringUtils::CompareNoCase(id, "changelog") == 0)
    str = addon->ChangeLog();
  else if (StringUtils::CompareNoCase(id, "description") == 0)
    str = addon->Description();
  else if (StringUtils::CompareNoCase(id, "disclaimer") == 0)
    str = addon->Disclaimer();
  else if (StringUtils::CompareNoCase(id, "fanart") == 0)
    str = addon->FanArt();
  else if (StringUtils::CompareNoCase(id, "icon") == 0)
    str = addon->Icon();
  else if (StringUtils::CompareNoCase(id, "id") == 0)
    str = addon->ID();
  else if (StringUtils::CompareNoCase(id, "name") == 0)
    str = addon->Name();
  else if (StringUtils::CompareNoCase(id, "path") == 0)
    str = addon->Path();
  else if (StringUtils::CompareNoCase(id, "profile") == 0)
    str = addon->Profile();
  else if (StringUtils::CompareNoCase(id, "summary") == 0)
    str = addon->Summary();
  else if (StringUtils::CompareNoCase(id, "type") == 0)
    str = ADDON::CAddonInfo::TranslateType(addon->Type());
  else if (StringUtils::CompareNoCase(id, "version") == 0)
    str = addon->Version().asString();
  else
  {
    CLog::Log(LOGERROR, "Interface_General::{} -  add-on '{}' requests invalid id '{}'",
              __FUNCTION__, addon->Name(), id);
    return nullptr;
  }

  char* buffer = strdup(str.c_str());
  return buffer;
}

bool Interface_General::open_settings_dialog(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
    return false;
  }

  // show settings dialog
  AddonPtr addonInfo;
  if (CServiceBroker::GetAddonMgr().GetAddon(addon->ID(), addonInfo))
  {
    CLog::Log(LOGERROR, "Interface_General::{} - Could not get addon information for '{}'",
              __FUNCTION__, addon->ID());
    return false;
  }

  return CGUIDialogAddonSettings::ShowForAddon(addonInfo);
}

char* Interface_General::get_localized_string(void* kodiBase, long label_id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
    return nullptr;
  }

  if (g_application.m_bStop)
    return nullptr;

  std::string label = g_localizeStrings.GetAddonString(addon->ID(), label_id);
  if (label.empty())
    label = g_localizeStrings.Get(label_id);
  char* buffer = strdup(label.c_str());
  return buffer;
}

char* Interface_General::unknown_to_utf8(void* kodiBase, const char* source, bool* ret, bool failOnBadChar)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !source || !ret)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', source='{}', ret='{}')",
              __FUNCTION__, kodiBase, static_cast<const void*>(source), static_cast<void*>(ret));
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
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
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
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', message='{}')",
              __FUNCTION__, kodiBase, static_cast<const void*>(message));
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
    case QueueMsg::QUEUE_WARNING:
      usedType = CGUIDialogKaiToast::Warning;
      withSound = true;
      CLog::Log(LOGDEBUG, "Interface_General::{} - {} - Warning Message: '{}'", __FUNCTION__,
                addon->Name(), message);
      break;
    case QueueMsg::QUEUE_ERROR:
      usedType = CGUIDialogKaiToast::Error;
      withSound = true;
      CLog::Log(LOGDEBUG, "Interface_General::{} - {} - Error Message : '{}'", __FUNCTION__,
                addon->Name(), message);
      break;
    case QueueMsg::QUEUE_INFO:
    default:
      usedType = CGUIDialogKaiToast::Info;
      withSound = false;
      CLog::Log(LOGDEBUG, "Interface_General::{} - {} - Info Message : '{}'", __FUNCTION__,
                addon->Name(), message);
      break;
    }

    if (imageFile && strlen(imageFile) > 0)
    {
      CLog::Log(LOGERROR,
                "Interface_General::{} - To use given image file '{}' must be type value set to "
                "'QUEUE_OWN_STYLE'",
                __FUNCTION__, imageFile);
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
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', text='{}')",
              __FUNCTION__, kodiBase, static_cast<const void*>(text));
    return;
  }

  std::string md5Int = CDigest::Calculate(CDigest::Type::MD5, std::string(text));
  strncpy(md5, md5Int.c_str(), 40);
}

char* Interface_General::get_temp_path(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - called with empty kodi instance pointer",
              __FUNCTION__);
    return nullptr;
  }

  std::string tempPath =
      URIUtils::AddFileToFolder(CServiceBroker::GetAddonMgr().GetTempAddonBasePath(), addon->ID());
  tempPath += "-temp";
  XFILE::CDirectory::Create(tempPath);

  return strdup(CSpecialProtocol::TranslatePath(tempPath).c_str());
}

char* Interface_General::get_region(void* kodiBase, const char* id)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', id='{}')", __FUNCTION__,
              kodiBase, static_cast<const void*>(id));
    return nullptr;
  }

  std::string result;
  if (StringUtils::CompareNoCase(id, "datelong") == 0)
  {
    result = g_langInfo.GetDateFormat(true);
    StringUtils::Replace(result, "DDDD", "%A");
    StringUtils::Replace(result, "MMMM", "%B");
    StringUtils::Replace(result, "D", "%d");
    StringUtils::Replace(result, "YYYY", "%Y");
  }
  else if (StringUtils::CompareNoCase(id, "dateshort") == 0)
  {
    result = g_langInfo.GetDateFormat(false);
    StringUtils::Replace(result, "MM", "%m");
    StringUtils::Replace(result, "DD", "%d");
#ifdef TARGET_WINDOWS
    StringUtils::Replace(result, "M", "%#m");
    StringUtils::Replace(result, "D", "%#d");
#else
    StringUtils::Replace(result, "M", "%-m");
    StringUtils::Replace(result, "D", "%-d");
#endif
    StringUtils::Replace(result, "YYYY", "%Y");
  }
  else if (StringUtils::CompareNoCase(id, "tempunit") == 0)
    result = g_langInfo.GetTemperatureUnitString();
  else if (StringUtils::CompareNoCase(id, "speedunit") == 0)
    result = g_langInfo.GetSpeedUnitString();
  else if (StringUtils::CompareNoCase(id, "time") == 0)
  {
    result = g_langInfo.GetTimeFormat();
    StringUtils::Replace(result, "H", "%H");
    StringUtils::Replace(result, "h", "%I");
    StringUtils::Replace(result, "mm", "%M");
    StringUtils::Replace(result, "ss", "%S");
    StringUtils::Replace(result, "xx", "%p");
  }
  else if (StringUtils::CompareNoCase(id, "meridiem") == 0)
    result = StringUtils::Format("%s/%s",
                                  g_langInfo.GetMeridiemSymbol(MeridiemSymbolAM).c_str(),
                                  g_langInfo.GetMeridiemSymbol(MeridiemSymbolPM).c_str());
  else
  {
    CLog::Log(LOGERROR, "Interface_General::{} -  add-on '{}' requests invalid id '{}'",
              __FUNCTION__, addon->Name(), id);
    return nullptr;
  }

  char* buffer = strdup(result.c_str());
  return buffer;
}

void Interface_General::get_free_mem(void* kodiBase, long* free, long* total, bool as_bytes)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || free == nullptr || total == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', free='{}', total='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(free), static_cast<void*>(total));
    return;
  }

  KODI::MEMORY::MemoryStatus stat;
  KODI::MEMORY::GetMemoryStatus(&stat);
  *free = static_cast<long>(stat.availPhys);
  *total = static_cast<long>(stat.totalPhys);
  if (!as_bytes)
  {
    *free = *free / ( 1024 * 1024 );
    *total = *total / ( 1024 * 1024 );
  }
}

int Interface_General::get_global_idle_time(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
    return -1;
  }

  return g_application.GlobalIdleTime();
}

void Interface_General::kodi_version(void* kodiBase, char** compile_name, int* major, int* minor, char** revision, char** tag, char** tagversion)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || compile_name == nullptr || major == nullptr || minor == nullptr ||
     revision == nullptr || tag == nullptr || tagversion == nullptr)
  {
    CLog::Log(LOGERROR,
              "Interface_General::{} - invalid data (addon='{}', compile_name='{}', major='{}', "
              "minor='{}', revision='{}', tag='{}', tagversion='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(compile_name), static_cast<void*>(major),
              static_cast<void*>(minor), static_cast<void*>(revision), static_cast<void*>(tag),
              static_cast<void*>(tagversion));
    return;
  }
    
  *compile_name = strdup(CCompileInfo::GetAppName());
  *major = CCompileInfo::GetMajor();
  *minor = CCompileInfo::GetMinor();
  *revision = strdup(CCompileInfo::GetSCMID());
  std::string tagStr = CCompileInfo::GetSuffix();
  if (StringUtils::StartsWithNoCase(tagStr, "alpha"))
  {
    *tag = strdup("alpha");
    *tagversion = strdup(StringUtils::Mid(tagStr, 5).c_str());
  }
  else if (StringUtils::StartsWithNoCase(tagStr, "beta"))
  {
    *tag = strdup("beta");
    *tagversion = strdup(StringUtils::Mid(tagStr, 4).c_str());
  }
  else if (StringUtils::StartsWithNoCase(tagStr, "rc"))
  {
    *tag = strdup("releasecandidate");
    *tagversion = strdup(StringUtils::Mid(tagStr, 2).c_str());
  }
  else if (tagStr.empty())
    *tag = strdup("stable");
  else
    *tag = strdup("prealpha");
}

char* Interface_General::get_current_skin_id(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
    return nullptr;
  }

  return strdup(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN).c_str());
}

bool Interface_General::get_keyboard_layout(void* kodiBase, char** layout_name, int modifier_key, AddonKeyboardKeyTable* c_layout)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || c_layout == nullptr || layout_name == nullptr)
  {
    CLog::Log(LOGERROR,
              "Interface_General::{} - invalid data (addon='{}', c_layout='{}', layout_name='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(c_layout),
              static_cast<void*>(layout_name));
    return false;
  }

  std::string activeLayout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT);

  CKeyboardLayout layout;
  if (!CKeyboardLayoutManager::GetInstance().GetLayout(activeLayout, layout))
    return false;

  *layout_name = strdup(layout.GetName().c_str());

  unsigned int modifiers = CKeyboardLayout::ModifierKeyNone;
  if (modifier_key & STD_KB_MODIFIER_KEY_SHIFT)
    modifiers |= CKeyboardLayout::ModifierKeyShift;
  if (modifier_key & STD_KB_MODIFIER_KEY_SYMBOL)
    modifiers |= CKeyboardLayout::ModifierKeySymbol;

  for (unsigned int row = 0; row < STD_KB_BUTTONS_MAX_ROWS; row++)
  {
    for (unsigned int column = 0; column < STD_KB_BUTTONS_PER_ROW; column++)
    {
      std::string label = layout.GetCharAt(row, column, modifiers);
      c_layout->keys[row][column] = strdup(label.c_str());
    }
  }

  return true;
}

bool Interface_General::change_keyboard_layout(void* kodiBase, char** layout_name)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || layout_name == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_General::{} - invalid data (addon='{}', layout_name='{}')",
              __FUNCTION__, kodiBase, static_cast<void*>(layout_name));
    return false;
  }

  std::vector<CKeyboardLayout> layouts;
  unsigned int currentLayout = 0;

  const KeyboardLayouts& keyboardLayouts = CKeyboardLayoutManager::GetInstance().GetLayouts();
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::vector<CVariant> layoutNames = settings->GetList(CSettings::SETTING_LOCALE_KEYBOARDLAYOUTS);
  std::string activeLayout = settings->GetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT);

  for (const auto& layoutName : layoutNames)
  {
    const auto keyboardLayout = keyboardLayouts.find(layoutName.asString());
    if (keyboardLayout != keyboardLayouts.end())
    {
      layouts.emplace_back(keyboardLayout->second);
      if (layoutName.asString() == activeLayout)
        currentLayout = layouts.size() - 1;
    }
  }

  currentLayout++;
  if (currentLayout >= layouts.size())
    currentLayout = 0;
  CKeyboardLayout layout = layouts.empty() ? CKeyboardLayout() : layouts[currentLayout];
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT, layout.GetName());

  *layout_name = strdup(layout.GetName().c_str());
  return true;
}

} /* namespace ADDON */
