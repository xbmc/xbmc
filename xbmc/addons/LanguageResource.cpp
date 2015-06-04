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
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

#define LANGUAGE_SETTING        "locale.language"
#define LANGUAGE_ADDON_PREFIX   "resource.language."

using namespace std;

namespace ADDON
{

CLanguageResource::CLanguageResource(const cp_extension_t *ext)
  : CResource(ext),
  m_forceUnicodeFont(false)
{
  if (ext != NULL)
  {
    // parse <extension> attributes
    std::string locale = CAddonMgr::Get().GetExtValue(ext->configuration, "@locale");
    m_locale = CLocale::FromString(locale);

    // parse <charsets>
    cp_cfg_element_t *charsetsElement = CAddonMgr::Get().GetExtElement(ext->configuration, "charsets");
    if (charsetsElement != NULL)
    {
      m_charsetGui = CAddonMgr::Get().GetExtValue(charsetsElement, "gui");
      m_forceUnicodeFont = CAddonMgr::Get().GetExtValue(charsetsElement, "gui@unicodefont") == "true";
      m_charsetSubtitle = CAddonMgr::Get().GetExtValue(charsetsElement, "subtitle");
    }

    // parse <dvd>
    cp_cfg_element_t *dvdElement = CAddonMgr::Get().GetExtElement(ext->configuration, "dvd");
    if (dvdElement != NULL)
    {
      m_dvdLanguageMenu = CAddonMgr::Get().GetExtValue(dvdElement, "menu");
      m_dvdLanguageAudio = CAddonMgr::Get().GetExtValue(dvdElement, "audio");
      m_dvdLanguageSubtitle = CAddonMgr::Get().GetExtValue(dvdElement, "subtitle");
    }
    // fall back to the language of the addon if a DVD language is not defined
    if (m_dvdLanguageMenu.empty())
      m_dvdLanguageMenu = m_locale.GetLanguageCode();
    if (m_dvdLanguageAudio.empty())
      m_dvdLanguageAudio = m_locale.GetLanguageCode();
    if (m_dvdLanguageSubtitle.empty())
      m_dvdLanguageSubtitle = m_locale.GetLanguageCode();

    // parse <sorttokens>
    cp_cfg_element_t *sorttokensElement = CAddonMgr::Get().GetExtElement(ext->configuration, "sorttokens");
    if (sorttokensElement != NULL)
    {
      for (size_t i = 0; i < sorttokensElement->num_children; ++i)
      {
        cp_cfg_element_t &tokenElement = sorttokensElement->children[i];
        if (tokenElement.name != NULL && strcmp(tokenElement.name, "token") == 0 &&
            tokenElement.value != NULL)
        {
          std::string token(tokenElement.value);
          std::string separators = CAddonMgr::Get().GetExtValue(&tokenElement, "@separators");
          if (separators.empty())
            separators = " ._";

          for (std::string::const_iterator separator = separators.begin(); separator != separators.end(); ++separator)
            m_sortTokens.insert(token + *separator);
        }
      }
    }
  }
}

CLanguageResource::CLanguageResource(const CLanguageResource &rhs)
  : CResource(rhs),
    m_locale(rhs.m_locale),
    m_forceUnicodeFont(rhs.m_forceUnicodeFont)
{ }

AddonPtr CLanguageResource::Clone() const
{
  return AddonPtr(new CLanguageResource(*this));
}

bool CLanguageResource::IsInUse() const
{
  return StringUtils::EqualsNoCase(CSettings::Get().GetString(LANGUAGE_SETTING), ID());
}

void CLanguageResource::OnPostInstall(bool update, bool modal)
{
  if (IsInUse() ||
     (!update && !modal && CGUIDialogYesNo::ShowAndGetInput(Name(), 24132)))
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
      CSettings::Get().SetString(LANGUAGE_SETTING, ID());
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
  if (!CAddonMgr::Get().GetAddon(addonId, addon, ADDON_RESOURCE_LANGUAGE, true))
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
  else if (!CAddonMgr::Get().GetAddons(ADDON_RESOURCE_LANGUAGE, addons, true) || addons.empty())
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
