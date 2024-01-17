/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "General.h"

#include "CompileInfo.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/General.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "input/keyboard/KeyboardLayout.h"
#include "input/keyboard/KeyboardLayoutManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/Digest.h"
#include "utils/LangCodeExpander.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <string.h>

using namespace kodi; // addon-dev-kit namespace
using namespace KODI;
using UTILITY::CDigest;

namespace ADDON
{

void Interface_General::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi = static_cast<AddonToKodiFuncTable_kodi*>(malloc(sizeof(AddonToKodiFuncTable_kodi)));

  addonInterface->toKodi->kodi->unknown_to_utf8 = unknown_to_utf8;
  addonInterface->toKodi->kodi->get_language = get_language;
  addonInterface->toKodi->kodi->queue_notification = queue_notification;
  addonInterface->toKodi->kodi->get_md5 = get_md5;
  addonInterface->toKodi->kodi->get_region = get_region;
  addonInterface->toKodi->kodi->get_free_mem = get_free_mem;
  addonInterface->toKodi->kodi->get_global_idle_time = get_global_idle_time;
  addonInterface->toKodi->kodi->is_addon_avilable = is_addon_avilable;
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
    result = StringUtils::Format("{}/{}", g_langInfo.GetMeridiemSymbol(MeridiemSymbolAM),
                                 g_langInfo.GetMeridiemSymbol(MeridiemSymbolPM));
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

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  return appPower->GlobalIdleTime();
}

bool Interface_General::is_addon_avilable(void* kodiBase,
                                          const char* id,
                                          char** version,
                                          bool* enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || id == nullptr || version == nullptr || enabled == nullptr)
  {
    CLog::Log(
        LOGERROR,
        "Interface_General::{} - invalid data (addon='{}', id='{}', version='{}', enabled='{}')",
        __FUNCTION__, kodiBase, static_cast<const void*>(id), static_cast<void*>(version),
        static_cast<void*>(enabled));
    return false;
  }

  AddonPtr addonInfo;
  if (!CServiceBroker::GetAddonMgr().GetAddon(id, addonInfo, OnlyEnabled::CHOICE_NO))
    return false;

  *version = strdup(addonInfo->Version().asString().c_str());
  *enabled = !CServiceBroker::GetAddonMgr().IsAddonDisabled(id);
  return true;
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

  KEYBOARD::CKeyboardLayout layout;
  if (!CServiceBroker::GetKeyboardLayoutManager()->GetLayout(activeLayout, layout))
    return false;

  *layout_name = strdup(layout.GetName().c_str());

  unsigned int modifiers = KEYBOARD::CKeyboardLayout::ModifierKeyNone;
  if (modifier_key & STD_KB_MODIFIER_KEY_SHIFT)
    modifiers |= KEYBOARD::CKeyboardLayout::ModifierKeyShift;
  if (modifier_key & STD_KB_MODIFIER_KEY_SYMBOL)
    modifiers |= KEYBOARD::CKeyboardLayout::ModifierKeySymbol;

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

  std::vector<KEYBOARD::CKeyboardLayout> layouts;
  unsigned int currentLayout = 0;

  const KEYBOARD::KeyboardLayouts& keyboardLayouts =
      CServiceBroker::GetKeyboardLayoutManager()->GetLayouts();
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
  KEYBOARD::CKeyboardLayout layout =
      layouts.empty() ? KEYBOARD::CKeyboardLayout() : layouts[currentLayout];
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_ACTIVEKEYBOARDLAYOUT, layout.GetName());

  *layout_name = strdup(layout.GetName().c_str());
  return true;
}

} /* namespace ADDON */
