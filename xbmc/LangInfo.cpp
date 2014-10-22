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

#include "LangInfo.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/LanguageResource.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#include <algorithm>

using namespace std;
using namespace PVR;

#define TEMP_UNIT_STRINGS 20027

#define SPEED_UNIT_STRINGS 20200

struct SortLanguage
{
  bool operator()(const std::pair<std::string, std::string> &left, const std::pair<std::string, std::string> &right)
  {
    std::string strLeft = left.first;
    std::string strRight = right.first;
    StringUtils::ToLower(strLeft);
    StringUtils::ToLower(strRight);

    return strLeft.compare(strRight) < 0;
  }
};

CLangInfo::CRegion::CRegion(const CRegion& region):
  m_strLangLocaleName(region.m_strLangLocaleName),
  m_strLangLocaleCodeTwoChar(region.m_strLangLocaleCodeTwoChar),
  m_strRegionLocaleName(region.m_strRegionLocaleName),
  m_strName(region.m_strName),
  m_strDateFormatLong(region.m_strDateFormatLong),
  m_strDateFormatShort(region.m_strDateFormatShort),
  m_strTimeFormat(region.m_strTimeFormat),
  m_strTimeZone(region.m_strTimeZone)
{
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM];
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM];
  m_tempUnit=region.m_tempUnit;
  m_speedUnit=region.m_speedUnit;
}

CLangInfo::CRegion::CRegion()
{
  SetDefaults();
}

CLangInfo::CRegion::~CRegion()
{

}

void CLangInfo::CRegion::SetDefaults()
{
  m_strName="N/A";
  m_strLangLocaleName = "English";
  m_strLangLocaleCodeTwoChar = "en";

  m_strDateFormatShort="DD/MM/YYYY";
  m_strDateFormatLong="DDDD, D MMMM YYYY";
  m_strTimeFormat="HH:mm:ss";
  m_tempUnit=TEMP_UNIT_CELSIUS;
  m_speedUnit=SPEED_UNIT_KMH;
  m_strTimeZone.clear();
}

void CLangInfo::CRegion::SetTempUnit(const std::string& strUnit)
{
  std::string unit(strUnit); StringUtils::ToLower(unit);
  if (unit == "f")
    m_tempUnit=TEMP_UNIT_FAHRENHEIT;
  else if (unit == "k")
    m_tempUnit=TEMP_UNIT_KELVIN;
  else if (unit == "c")
    m_tempUnit=TEMP_UNIT_CELSIUS;
  else if (unit == "re")
    m_tempUnit=TEMP_UNIT_REAUMUR;
  else if (unit == "ra")
    m_tempUnit=TEMP_UNIT_RANKINE;
  else if (unit == "ro")
    m_tempUnit=TEMP_UNIT_ROMER;
  else if (unit == "de")
    m_tempUnit=TEMP_UNIT_DELISLE;
  else if (unit == "n")
    m_tempUnit=TEMP_UNIT_NEWTON;
}

void CLangInfo::CRegion::SetSpeedUnit(const std::string& strUnit)
{
  std::string unit(strUnit); StringUtils::ToLower(unit);
  if (unit == "kmh")
    m_speedUnit=SPEED_UNIT_KMH;
  else if (unit == "mpmin")
    m_speedUnit=SPEED_UNIT_MPMIN;
  else if (unit == "mps")
    m_speedUnit=SPEED_UNIT_MPS;
  else if (unit == "fth")
    m_speedUnit=SPEED_UNIT_FTH;
  else if (unit == "ftm")
    m_speedUnit=SPEED_UNIT_FTMIN;
  else if (unit == "fts")
    m_speedUnit=SPEED_UNIT_FTS;
  else if (unit == "mph")
    m_speedUnit=SPEED_UNIT_MPH;
  else if (unit == "kts")
    m_speedUnit=SPEED_UNIT_KTS;
  else if (unit == "beaufort")
    m_speedUnit=SPEED_UNIT_BEAUFORT;
  else if (unit == "inchs")
    m_speedUnit=SPEED_UNIT_INCHPS;
  else if (unit == "yards")
    m_speedUnit=SPEED_UNIT_YARDPS;
  else if (unit == "fpf")
    m_speedUnit=SPEED_UNIT_FPF;
}

void CLangInfo::CRegion::SetTimeZone(const std::string& strTimeZone)
{
  m_strTimeZone = strTimeZone;
}

// set the locale associated with this region global. This affects string
// sorting & transformations
void CLangInfo::CRegion::SetGlobalLocale()
{
  std::string strLocale;
  if (m_strRegionLocaleName.length() > 0)
  {
    strLocale = m_strLangLocaleName + "_" + m_strRegionLocaleName;
#ifdef TARGET_POSIX
    strLocale += ".UTF-8";
#endif
  }

  CLog::Log(LOGDEBUG, "trying to set locale to %s", strLocale.c_str());

  // We need to set the locale to only change the collate. Otherwise,
  // decimal separator is changed depending of the current language
  // (ie. "," in French or Dutch instead of "."). This breaks atof() and
  // others similar functions.
#if defined(TARGET_FREEBSD) || defined(TARGET_DARWIN_OSX) || defined(__UCLIBC__)
  // on FreeBSD, darwin and uClibc-based systems libstdc++ is compiled with
  // "generic" locale support
  if (setlocale(LC_COLLATE, strLocale.c_str()) == NULL
  || setlocale(LC_CTYPE, strLocale.c_str()) == NULL)
  {
    strLocale = "C";
    setlocale(LC_COLLATE, strLocale.c_str());
    setlocale(LC_CTYPE, strLocale.c_str());
  }
#else
  locale current_locale = locale::classic(); // C-Locale
  try
  {
    locale lcl = locale(strLocale.c_str());
    strLocale = lcl.name();
    current_locale = current_locale.combine< collate<wchar_t> >(lcl);
    current_locale = current_locale.combine< ctype<wchar_t> >(lcl);

    assert(use_facet< numpunct<char> >(current_locale).decimal_point() == '.');

  } catch(...) {
    current_locale = locale::classic();
    strLocale = "C";
  }

  g_langInfo.m_systemLocale = current_locale; // TODO: move to CLangInfo class
  locale::global(current_locale);
#endif
  g_charsetConverter.resetSystemCharset();
  CLog::Log(LOGINFO, "global locale set to %s", strLocale.c_str());
}

CLangInfo::CLangInfo()
{
  SetDefaults();
}

CLangInfo::~CLangInfo()
{
}

void CLangInfo::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "locale.audiolanguage")
    SetAudioLanguage(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.subtitlelanguage")
    SetSubtitleLanguage(((CSettingString*)setting)->GetValue());
  else if (settingId == "locale.language")
  {
    if (!SetLanguage(((CSettingString*)setting)->GetValue()))
      ((CSettingString*)CSettings::Get().GetSetting("locale.language"))->Reset();
  }
  else if (settingId == "locale.country")
  {
    g_langInfo.SetCurrentRegion(((CSettingString*)setting)->GetValue());
    g_weatherManager.Refresh(); // need to reset our weather, as temperatures need re-translating.
  }
}

bool CLangInfo::Load(const std::string& strLanguage, bool onlyCheckLanguage /*= false*/)
{
  SetDefaults();

  string strFileName = GetLanguageInfoPath(strLanguage);
  if (!onlyCheckLanguage)
    CLog::Log(LOGINFO, "CLangInfo: load language info file: %s", strFileName.c_str());

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strFileName))
  {
    CLog::Log(onlyCheckLanguage ? LOGDEBUG : LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  // get the matching language addon
  m_languageAddon = GetLanguageAddon(strLanguage);
  if (m_languageAddon == NULL)
  {
    CLog::Log(onlyCheckLanguage ? LOGDEBUG : LOGERROR, "Unknown language %s", strLanguage.c_str());
    return false;
  }

  if (!onlyCheckLanguage)
  {
    // get some language-specific information from the language addon
    m_strGuiCharSet = m_languageAddon->GetGuiCharset();
    m_forceUnicodeFont = m_languageAddon->ForceUnicodeFont();
    m_strSubtitleCharSet = m_languageAddon->GetSubtitleCharset();
    m_strDVDMenuLanguage = m_languageAddon->GetDvdMenuLanguage();
    m_strDVDAudioLanguage = m_languageAddon->GetDvdAudioLanguage();
    m_strDVDSubtitleLanguage = m_languageAddon->GetDvdSubtitleLanguage();
    m_sortTokens = m_languageAddon->GetSortTokens();
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->ValueStr() != "language")
  {
    CLog::Log(onlyCheckLanguage ? LOGDEBUG : LOGERROR, "%s Doesn't contain <language>", strFileName.c_str());
    return false;
  }

  if (pRootElement->Attribute("locale"))
    m_defaultRegion.m_strLangLocaleName = pRootElement->Attribute("locale");

#ifdef TARGET_WINDOWS
  // Windows need 3 chars isolang code
  if (m_defaultRegion.m_strLangLocaleName.length() == 2)
  {
    if (! g_LangCodeExpander.ConvertTwoToThreeCharCode(m_defaultRegion.m_strLangLocaleName, m_defaultRegion.m_strLangLocaleName, true))
      m_defaultRegion.m_strLangLocaleName = "";
  }

  if (!g_LangCodeExpander.ConvertWindowsToGeneralCharCode(m_defaultRegion.m_strLangLocaleName, m_languageCodeGeneral))
    m_languageCodeGeneral = "";
#else
  if (m_defaultRegion.m_strLangLocaleName.length() != 3)
  {
    if (!g_LangCodeExpander.ConvertToThreeCharCode(m_languageCodeGeneral, m_defaultRegion.m_strLangLocaleName, !onlyCheckLanguage))
      m_languageCodeGeneral = "";
  }
  else
    m_languageCodeGeneral = m_defaultRegion.m_strLangLocaleName;
#endif

  std::string tmp;
  if (g_LangCodeExpander.ConvertToTwoCharCode(tmp, m_defaultRegion.m_strLangLocaleName))
    m_defaultRegion.m_strLangLocaleCodeTwoChar = tmp;

  const TiXmlNode *pRegions = pRootElement->FirstChild("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const TiXmlElement *pRegion=pRegions->FirstChildElement("region");
    while (pRegion)
    {
      CRegion region(m_defaultRegion);
      region.m_strName = XMLUtils::GetAttribute(pRegion, "name");
      if (region.m_strName.empty())
        region.m_strName="N/A";

      if (pRegion->Attribute("locale"))
        region.m_strRegionLocaleName = pRegion->Attribute("locale");

#ifdef TARGET_WINDOWS
      // Windows need 3 chars regions code
      if (region.m_strRegionLocaleName.length() == 2)
      {
        if (! g_LangCodeExpander.ConvertLinuxToWindowsRegionCodes(region.m_strRegionLocaleName, region.m_strRegionLocaleName))
          region.m_strRegionLocaleName = "";
      }
#endif

      const TiXmlNode *pDateLong=pRegion->FirstChild("datelong");
      if (pDateLong && !pDateLong->NoChildren())
        region.m_strDateFormatLong=pDateLong->FirstChild()->ValueStr();

      const TiXmlNode *pDateShort=pRegion->FirstChild("dateshort");
      if (pDateShort && !pDateShort->NoChildren())
        region.m_strDateFormatShort=pDateShort->FirstChild()->ValueStr();

      const TiXmlElement *pTime=pRegion->FirstChildElement("time");
      if (pTime && !pTime->NoChildren())
      {
        region.m_strTimeFormat=pTime->FirstChild()->Value();
        region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM] = XMLUtils::GetAttribute(pTime, "symbolAM");
        region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM] = XMLUtils::GetAttribute(pTime, "symbolPM");
      }

      const TiXmlNode *pTempUnit=pRegion->FirstChild("tempunit");
      if (pTempUnit && !pTempUnit->NoChildren())
        region.SetTempUnit(pTempUnit->FirstChild()->ValueStr());

      const TiXmlNode *pSpeedUnit=pRegion->FirstChild("speedunit");
      if (pSpeedUnit && !pSpeedUnit->NoChildren())
        region.SetSpeedUnit(pSpeedUnit->FirstChild()->ValueStr());

      const TiXmlNode *pTimeZone=pRegion->FirstChild("timezone");
      if (pTimeZone && !pTimeZone->NoChildren())
        region.SetTimeZone(pTimeZone->FirstChild()->ValueStr());

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion=pRegion->NextSiblingElement("region");
    }

    if (!onlyCheckLanguage)
    {
      const std::string& strName = CSettings::Get().GetString("locale.country");
      SetCurrentRegion(strName);
    }
  }
  g_charsetConverter.reinitCharsetsFromSettings();

  return true;
}

std::string CLangInfo::GetLanguagePath(const std::string &language)
{
  if (language.empty())
    return "";

  std::string addonId = ADDON::CLanguageResource::GetAddonId(language);

  std::string path = URIUtils::AddFileToFolder(GetLanguagePath(), addonId);
  URIUtils::AddSlashAtEnd(path);

  return path;
}

std::string CLangInfo::GetLanguageInfoPath(const std::string &language)
{
  if (language.empty())
    return "";

  return URIUtils::AddFileToFolder(GetLanguagePath(language), "langinfo.xml");
}

bool CLangInfo::CheckLanguage(const std::string& language)
{
  CLangInfo li;
  return li.Load(language, true);
}

void CLangInfo::LoadTokens(const TiXmlNode* pTokens, set<std::string>& vecTokens)
{
  if (pTokens && !pTokens->NoChildren())
  {
    const TiXmlElement *pToken = pTokens->FirstChildElement("token");
    while (pToken)
    {
      std::string strSep= " ._";
      if (pToken->Attribute("separators"))
        strSep = pToken->Attribute("separators");
      if (pToken->FirstChild() && pToken->FirstChild()->Value())
      {
        if (strSep.empty())
          vecTokens.insert(pToken->FirstChild()->ValueStr());
        else
          for (unsigned int i=0;i<strSep.size();++i)
            vecTokens.insert(pToken->FirstChild()->ValueStr() + strSep[i]);
      }
      pToken = pToken->NextSiblingElement();
    }
  }
}

void CLangInfo::SetDefaults()
{
  m_regions.clear();

  //Reset default region
  m_defaultRegion.SetDefaults();

  // Set the default region, we may be unable to load langinfo.xml
  m_currentRegion = &m_defaultRegion;

  m_systemLocale = std::locale::classic();

  m_forceUnicodeFont = false;
  m_strGuiCharSet = "CP1252";
  m_strSubtitleCharSet = "CP1252";
  m_strDVDMenuLanguage = "en";
  m_strDVDAudioLanguage = "en";
  m_strDVDSubtitleLanguage = "en";
  m_sortTokens.clear();
  
  m_languageCodeGeneral = "eng";
}

std::string CLangInfo::GetGuiCharSet() const
{
  CSettingString* charsetSetting = static_cast<CSettingString*>(CSettings::Get().GetSetting("locale.charset"));
  if (charsetSetting->IsDefault())
    return m_strGuiCharSet;

  return charsetSetting->GetValue();
}

std::string CLangInfo::GetSubtitleCharSet() const
{
  CSettingString* charsetSetting = static_cast<CSettingString*>(CSettings::Get().GetSetting("subtitles.charset"));
  if (charsetSetting->IsDefault())
    return m_strSubtitleCharSet;

  return charsetSetting->GetValue();
}

LanguageResourcePtr CLangInfo::GetLanguageAddon(const std::string& locale /* = "" */) const
{
  if (locale.empty() ||
     (m_languageAddon != NULL && (locale.compare(m_languageAddon->ID()) == 0 || m_languageAddon->GetLocale().Equals(locale))))
    return m_languageAddon;

  std::string addonId = ADDON::CLanguageResource::GetAddonId(locale);
  if (addonId.empty())
    addonId = CSettings::Get().GetString("locale.language");

  ADDON::AddonPtr addon;
  if (ADDON::CAddonMgr::Get().GetAddon(addonId, addon, ADDON::ADDON_RESOURCE_LANGUAGE, true) && addon != NULL)
    return std::dynamic_pointer_cast<ADDON::CLanguageResource>(addon);

  return NULL;
}

std::string CLangInfo::GetEnglishLanguageName(const std::string& locale /* = "" */) const
{
  LanguageResourcePtr addon = GetLanguageAddon(locale);
  if (addon == NULL)
    return "";

  return addon->Name();
}

bool CLangInfo::SetLanguage(const std::string &strLanguage /* = "" */, bool reloadServices /* = true */)
{
  bool fallback;
  return SetLanguage(fallback, strLanguage, reloadServices);
}

bool CLangInfo::SetLanguage(bool& fallback, const std::string &strLanguage /* = "" */, bool reloadServices /* = true */)
{
  fallback = false;

  std::string language = strLanguage;
  if (language.empty())
  {
    language = CSettings::Get().GetString("locale.language");

    if (language.empty())
    {
      CLog::Log(LOGFATAL, "CLangInfo: cannot load empty language.");
      return false;
    }
  }

  LanguageResourcePtr languageAddon = GetLanguageAddon(language);
  if (languageAddon == NULL)
  {
    CLog::Log(LOGINFO, "CLangInfo: unable to load language \"%s\". Trying to determine matching language addon...", language.c_str());

    // we may have to fall back to the default language
    std::string defaultLanguage = static_cast<CSettingString*>(CSettings::Get().GetSetting("locale.language"))->GetDefault();
    std::string newLanguage = defaultLanguage;

    // try to determine a language addon matching the given language in name
    if (!ADDON::CLanguageResource::FindLanguageAddonByName(language, newLanguage))
    {
      CLog::Log(LOGERROR, "CLangInfo: unable to find language addon matching \"%s\". Falling back to default language.", language.c_str());

      CAddonDatabase addondb;
      if (addondb.Open())
      {
        // update the addon repositories to check if there's a matching language addon available for download
        CAddonInstaller::Get().UpdateRepos(true, true);

        ADDON::VECADDONS languageAddons;
        if (addondb.GetAddons(languageAddons, ADDON::ADDON_RESOURCE_LANGUAGE) && !languageAddons.empty())
        {
          // try to get the proper language addon by its name from all available language addons
          if (ADDON::CLanguageResource::FindLanguageAddonByName(language, newLanguage, languageAddons))
          {
            if (CAddonInstaller::Get().Install(newLanguage, true, "", false, false))
              CLog::Log(LOGINFO, "CLangInfo: successfully installed language addon \"%s\" matching current language \"%s\"", newLanguage.c_str(), language.c_str());
            else
              CLog::Log(LOGERROR, "CLangInfo: failed to installed language addon \"%s\" matching current language \"%s\"", newLanguage.c_str(), language.c_str());
          }
          else
            CLog::Log(LOGERROR, "CLangInfo: unable to match old language \"%s\" to any available language addon", language.c_str());
        }
        else
          CLog::Log(LOGERROR, "CLangInfo: no language addons available to match against \"%s\"", language.c_str());
      }
      else
        CLog::Log(LOGERROR, "CLangInfo: unable to open addon database to look for a language addon matching \"%s\"", language.c_str());
    }

    // if the new language matches the default language we are loading the
    // default language as a fallback
    if (newLanguage == defaultLanguage)
    {
      CLog::Log(LOGINFO, "CLangInfo: fall back to the default language \"%s\"", defaultLanguage.c_str());
      fallback = true;
    }

    if (!CSettings::Get().SetString("locale.language", newLanguage))
      return false;

    CSettings::Get().Save();
    return true;
  }

  CLog::Log(LOGINFO, "CLangInfo: loading %s language information...", language.c_str());
  if (!Load(language))
  {
    CLog::LogF(LOGFATAL, "CLangInfo: failed to load %s language information", language.c_str());
    return false;
  }

  CLog::Log(LOGINFO, "CLangInfo: loading %s language strings...", language.c_str());
  if (!g_localizeStrings.Load(GetLanguagePath(), language))
  {
    CLog::LogF(LOGFATAL, "CLangInfo: failed to load %s language strings", language.c_str());
    return false;
  }

  if (reloadServices)
  {
    // also tell our weather and skin to reload as these are localized
    g_weatherManager.Refresh();
    g_PVRManager.LocalizationChanged();
    CApplicationMessenger::Get().ExecBuiltIn("ReloadSkin", false);
  }

  return true;
}

bool CLangInfo::CheckLoadLanguage(const std::string &language)
{
  return Load(language, true);
}

// three char language code (not win32 specific)
const std::string& CLangInfo::GetAudioLanguage() const
{
  if (!m_audioLanguage.empty())
    return m_audioLanguage;

  return m_languageCodeGeneral;
}

void CLangInfo::SetAudioLanguage(const std::string& language)
{
  if (language.empty()
    || StringUtils::EqualsNoCase(language, "default")
    || StringUtils::EqualsNoCase(language, "original")
    || !g_LangCodeExpander.ConvertToThreeCharCode(m_audioLanguage, language))
    m_audioLanguage.clear();
}

// three char language code (not win32 specific)
const std::string& CLangInfo::GetSubtitleLanguage() const
{
  if (!m_subtitleLanguage.empty())
    return m_subtitleLanguage;

  return m_languageCodeGeneral;
}

void CLangInfo::SetSubtitleLanguage(const std::string& language)
{
  if (language.empty()
    || StringUtils::EqualsNoCase(language, "default")
    || StringUtils::EqualsNoCase(language, "original")
    || !g_LangCodeExpander.ConvertToThreeCharCode(m_subtitleLanguage, language))
    m_subtitleLanguage.clear();
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDMenuLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToTwoCharCode(code, m_currentRegion->m_strLangLocaleName))
    code = m_strDVDMenuLanguage;
  
  return code;
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDAudioLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToTwoCharCode(code, m_audioLanguage))
    code = m_strDVDAudioLanguage;
  
  return code;
}

// two character codes as defined in ISO639
const std::string CLangInfo::GetDVDSubtitleLanguage() const
{
  std::string code;
  if (!g_LangCodeExpander.ConvertToTwoCharCode(code, m_subtitleLanguage))
    code = m_strDVDSubtitleLanguage;
  
  return code;
}

const CLocale& CLangInfo::GetLocale() const
{
  LanguageResourcePtr language = GetLanguageAddon();
  if (language != NULL)
    return language->GetLocale();

  return CLocale::Empty;
}

const std::string& CLangInfo::GetRegionLocale() const
{
  return m_currentRegion->m_strRegionLocaleName;
}

// Returns the format string for the date of the current language
const std::string& CLangInfo::GetDateFormat(bool bLongDate/*=false*/) const
{
  if (bLongDate)
    return m_currentRegion->m_strDateFormatLong;
  else
    return m_currentRegion->m_strDateFormatShort;
}

// Returns the format string for the time of the current language
const std::string& CLangInfo::GetTimeFormat() const
{
  return m_currentRegion->m_strTimeFormat;
}

const std::string& CLangInfo::GetTimeZone() const
{
  return m_currentRegion->m_strTimeZone;
}

// Returns the AM/PM symbol of the current language
const std::string& CLangInfo::GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const
{
  return m_currentRegion->m_strMeridiemSymbols[symbol];
}

// Fills the array with the region names available for this language
void CLangInfo::GetRegionNames(vector<string>& array)
{
  for (ITMAPREGIONS it=m_regions.begin(); it!=m_regions.end(); ++it)
  {
    std::string strName=it->first;
    if (strName=="N/A")
      strName=g_localizeStrings.Get(416);
    array.push_back(strName);
  }
}

// Set the current region by its name, names from GetRegionNames() are valid.
// If the region is not found the first available region is set.
void CLangInfo::SetCurrentRegion(const std::string& strName)
{
  ITMAPREGIONS it=m_regions.find(strName);
  if (it!=m_regions.end())
    m_currentRegion=&it->second;
  else if (!m_regions.empty())
    m_currentRegion=&m_regions.begin()->second;
  else
    m_currentRegion=&m_defaultRegion;

  m_currentRegion->SetGlobalLocale();
}

// Returns the current region set for this language
const std::string& CLangInfo::GetCurrentRegion() const
{
  return m_currentRegion->m_strName;
}

CLangInfo::TEMP_UNIT CLangInfo::GetTempUnit() const
{
  return m_currentRegion->m_tempUnit;
}

// Returns the temperature unit string for the current language
const std::string& CLangInfo::GetTempUnitString() const
{
  return g_localizeStrings.Get(TEMP_UNIT_STRINGS+m_currentRegion->m_tempUnit);
}

CLangInfo::SPEED_UNIT CLangInfo::GetSpeedUnit() const
{
  return m_currentRegion->m_speedUnit;
}

// Returns the speed unit string for the current language
const std::string& CLangInfo::GetSpeedUnitString() const
{
  return g_localizeStrings.Get(SPEED_UNIT_STRINGS+m_currentRegion->m_speedUnit);
}

std::set<std::string> CLangInfo::GetSortTokens() const
{
  std::set<std::string> sortTokens = m_sortTokens;
  sortTokens.insert(g_advancedSettings.m_vecTokens.begin(), g_advancedSettings.m_vecTokens.end());

  return sortTokens;
}

void CLangInfo::SettingOptionsLanguageNamesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  // find languages...
  ADDON::VECADDONS addons;
  if (!ADDON::CAddonMgr::Get().GetAddons(ADDON::ADDON_RESOURCE_LANGUAGE, addons, true))
    return;

  for (ADDON::VECADDONS::const_iterator addon = addons.begin(); addon != addons.end(); ++addon)
    list.push_back(make_pair((*addon)->Name(), (*addon)->Name()));

  sort(list.begin(), list.end(), SortLanguage());
}

void CLangInfo::SettingOptionsStreamLanguagesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  list.push_back(make_pair(g_localizeStrings.Get(308), "original"));
  list.push_back(make_pair(g_localizeStrings.Get(309), "default"));

  // get a list of language names
  vector<string> languages = g_LangCodeExpander.GetLanguageNames();
  sort(languages.begin(), languages.end(), sortstringbyname());
  for (std::vector<std::string>::const_iterator language = languages.begin(); language != languages.end(); ++language)
    list.push_back(make_pair(*language, *language));
}

void CLangInfo::SettingOptionsRegionsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  vector<string> regions;
  g_langInfo.GetRegionNames(regions);
  sort(regions.begin(), regions.end(), sortstringbyname());

  bool match = false;
  for (unsigned int i = 0; i < regions.size(); ++i)
  {
    std::string region = regions[i];
    list.push_back(make_pair(region, region));

    if (!match && region == ((CSettingString*)setting)->GetValue())
    {
      match = true;
      current = region;
    }
  }

  if (!match && regions.size() > 0)
    current = regions[0];
}
