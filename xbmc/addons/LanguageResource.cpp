/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://xbmc.org
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
#include "LanguageResource.h"
#include "LangInfo.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "messaging/helpers/DialogHelper.h"

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

#define LANGUAGE_ADDON_PREFIX   "resource.language."

namespace ADDON
{

std::unique_ptr<CLanguageResource> CLanguageResource::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  // parse <extension> attributes
  CLocale locale = CLocale::FromString(CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@locale"));

  // parse <charsets>
  std::string charsetGui;
  bool forceUnicodeFont(false);
  std::string charsetSubtitle;
  cp_cfg_element_t *charsetsElement = CAddonMgr::GetInstance().GetExtElement(ext->configuration, "charsets");
  if (charsetsElement != NULL)
  {
    charsetGui = CAddonMgr::GetInstance().GetExtValue(charsetsElement, "gui");
    forceUnicodeFont = CAddonMgr::GetInstance().GetExtValue(charsetsElement, "gui@unicodefont") == "true";
    charsetSubtitle = CAddonMgr::GetInstance().GetExtValue(charsetsElement, "subtitle");
  }

  // parse <dvd>
  std::string dvdLanguageMenu;
  std::string dvdLanguageAudio;
  std::string dvdLanguageSubtitle;
  cp_cfg_element_t *dvdElement = CAddonMgr::GetInstance().GetExtElement(ext->configuration, "dvd");
  if (dvdElement != NULL)
  {
    dvdLanguageMenu = CAddonMgr::GetInstance().GetExtValue(dvdElement, "menu");
    dvdLanguageAudio = CAddonMgr::GetInstance().GetExtValue(dvdElement, "audio");
    dvdLanguageSubtitle = CAddonMgr::GetInstance().GetExtValue(dvdElement, "subtitle");
  }
  // fall back to the language of the addon if a DVD language is not defined
  if (dvdLanguageMenu.empty())
    dvdLanguageMenu = locale.GetLanguageCode();
  if (dvdLanguageAudio.empty())
    dvdLanguageAudio = locale.GetLanguageCode();
  if (dvdLanguageSubtitle.empty())
    dvdLanguageSubtitle = locale.GetLanguageCode();

  // parse <sorttokens>
  std::set<std::string> sortTokens;
  cp_cfg_element_t *sorttokensElement = CAddonMgr::GetInstance().GetExtElement(ext->configuration, "sorttokens");
  if (sorttokensElement != NULL)
  {
    for (size_t i = 0; i < sorttokensElement->num_children; ++i)
    {
      cp_cfg_element_t &tokenElement = sorttokensElement->children[i];
      if (tokenElement.name != NULL && strcmp(tokenElement.name, "token") == 0 &&
          tokenElement.value != NULL)
      {
        std::string token(tokenElement.value);
        std::string separators = CAddonMgr::GetInstance().GetExtValue(&tokenElement, "@separators");
        if (separators.empty())
          separators = " ._";

        for (std::string::const_iterator separator = separators.begin(); separator != separators.end(); ++separator)
          sortTokens.insert(token + *separator);
      }
    }
  }
  return std::unique_ptr<CLanguageResource>(new CLanguageResource(
      std::move(props),
      locale,
      charsetGui,
      forceUnicodeFont,
      charsetSubtitle,
      dvdLanguageMenu,
      dvdLanguageAudio,
      dvdLanguageSubtitle,
      sortTokens));
}

CLanguageResource::CLanguageResource(
    AddonProps props,
    const CLocale& locale,
    const std::string& charsetGui,
    bool forceUnicodeFont,
    const std::string& charsetSubtitle,
    const std::string& dvdLanguageMenu,
    const std::string& dvdLanguageAudio,
    const std::string& dvdLanguageSubtitle,
    const std::set<std::string>& sortTokens)
  : CResource(std::move(props)),
    m_locale(locale),
    m_charsetGui(charsetGui),
    m_forceUnicodeFont(forceUnicodeFont),
    m_charsetSubtitle(charsetSubtitle),
    m_dvdLanguageMenu(dvdLanguageMenu),
    m_dvdLanguageAudio(dvdLanguageAudio),
    m_dvdLanguageSubtitle(dvdLanguageSubtitle),
    m_sortTokens(sortTokens)
{ }

bool CLanguageResource::IsInUse() const
{
  return StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_LANGUAGE), ID());
}

void CLanguageResource::OnPostInstall(bool update, bool modal)
{
  if (IsInUse() ||
     (!update && !modal && 
       (HELPERS::ShowYesNoDialogText(CVariant{Name()}, CVariant{24132}) == DialogResponse::YES)))
  {
    CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
    if (toast)
    {
      toast->ResetTimer();
      toast->Close(true);
    }

    if (IsInUse())
      g_langInfo.SetLanguage(ID());
    else
      CSettings::GetInstance().SetString(CSettings::SETTING_LOCALE_LANGUAGE, ID());
  }
}

bool CLanguageResource::IsAllowed(const std::string &file) const
{
  return file.empty() ||
         StringUtils::EqualsNoCase(file.c_str(), "langinfo.xml") ||
         StringUtils::EqualsNoCase(file.c_str(), "strings.po") ||
         StringUtils::EqualsNoCase(file.c_str(), "strings.xml");
}

std::string CLanguageResource::GetAddonId(const std::string& locale)
{
  if (locale.empty())
    return "";

  std::string addonId = locale;
  if (!StringUtils::StartsWith(addonId, LANGUAGE_ADDON_PREFIX))
    addonId = LANGUAGE_ADDON_PREFIX + locale;

  StringUtils::ToLower(addonId);
  return addonId;
}

bool CLanguageResource::FindLegacyLanguage(const std::string &locale, std::string &legacyLanguage)
{
  if (locale.empty())
    return false;

  std::string addonId = GetAddonId(locale);

  AddonPtr addon;
  if (!CAddonMgr::GetInstance().GetAddon(addonId, addon, ADDON_RESOURCE_LANGUAGE, true))
    return false;

  legacyLanguage = addon->Name();
  return true;
}

bool CLanguageResource::FindLanguageAddonByName(const std::string &legacyLanguage, std::string &addonId, const VECADDONS &languageAddons /* = VECADDONS() */)
{
  if (legacyLanguage.empty())
    return false;

  VECADDONS addons;
  if (!languageAddons.empty())
    addons = languageAddons;
  else if (!CAddonMgr::GetInstance().GetInstalledAddons(addons, ADDON_RESOURCE_LANGUAGE) || addons.empty())
    return false;

  // try to find a language that matches the old language in name or id
  for (VECADDONS::const_iterator addon = addons.begin(); addon != addons.end(); ++addon)
  {
    const CLanguageResource* languageAddon = static_cast<CLanguageResource*>(addon->get());

    // check if the old language matches the language addon id, the language
    // locale or the language addon name
    if (legacyLanguage.compare((*addon)->ID()) == 0 ||
        languageAddon->GetLocale().Equals(legacyLanguage) ||
        StringUtils::EqualsNoCase(legacyLanguage, languageAddon->Name()))
    {
      addonId = (*addon)->ID();
      return true;
    }
  }

  return false;
}

}
