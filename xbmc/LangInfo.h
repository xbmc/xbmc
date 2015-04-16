#pragma once
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

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/GlobalsHandling.h"
#include "utils/Locale.h"
#include "utils/Speed.h"
#include "utils/Temperature.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <locale>

#ifdef TARGET_WINDOWS
#ifdef GetDateFormat
#undef GetDateFormat
#endif // GetDateFormat
#ifdef GetTimeFormat
#undef GetTimeFormat
#endif // GetTimeFormat
#endif // TARGET_WINDOWS

class TiXmlNode;

namespace ADDON
{
  class CLanguageResource;
}
typedef std::shared_ptr<ADDON::CLanguageResource> LanguageResourcePtr;

typedef enum MeridiemSymbol
{
  MeridiemSymbolPM = 0,
  MeridiemSymbolAM
} MeridiemSymbol;

class CLangInfo : public ISettingCallback, public ISettingsHandler
{
public:
  CLangInfo();
  virtual ~CLangInfo();

  // implementation of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);

  // implementation of ISettingsHandler
  virtual void OnSettingsLoaded();

  bool Load(const std::string& strLanguage);

  /*!
   \brief Returns the language addon for the given locale (or the current one).

   \param locale (optional) Locale of the language (current if empty)
   \return Language addon for the given locale or NULL if the locale is invalid.
   */
  LanguageResourcePtr GetLanguageAddon(const std::string& locale = "") const;

  std::string GetGuiCharSet() const;
  std::string GetSubtitleCharSet() const;

  // three char language code (not win32 specific)
  const std::string& GetLanguageCode() const { return m_languageCodeGeneral; }

  /*!
   \brief Returns the given language's name in English

   \param locale (optional) Locale of the language (current if empty)
   */
  std::string GetEnglishLanguageName(const std::string& locale = "") const;

  /*!
  \brief Sets and loads the given (or configured) language, its details and strings.

  \param strLanguage (optional) Language to be loaded.
  \param reloadServices (optional) Whether to reload services relying on localization.
  \return True if the language has been successfully loaded, false otherwise.
  */
  bool SetLanguage(const std::string &strLanguage = "", bool reloadServices = true);
  /*!
   \brief Sets and loads the given (or configured) language, its details and strings.

   \param fallback Whether the fallback language has been loaded instead of the given language.
   \param strLanguage (optional) Language to be loaded.
   \param reloadServices (optional) Whether to reload services relying on localization.
   \return True if the language has been successfully loaded, false otherwise.
   */
  bool SetLanguage(bool& fallback, const std::string &strLanguage = "", bool reloadServices = true);

  const std::string& GetAudioLanguage() const;
  // language can either be a two char language code as defined in ISO639
  // or a three char language code
  // or a language name in english (as used by XBMC)
  void SetAudioLanguage(const std::string& language);
  
  // three char language code (not win32 specific)
  const std::string& GetSubtitleLanguage() const;
  // language can either be a two char language code as defined in ISO639
  // or a three char language code
  // or a language name in english (as used by XBMC)
  void SetSubtitleLanguage(const std::string& language);

  const std::string GetDVDMenuLanguage() const;
  const std::string GetDVDAudioLanguage() const;
  const std::string GetDVDSubtitleLanguage() const;
  const std::string& GetTimeZone() const;

  const std::string& GetRegionLocale() const;

  /*!
  \brief Returns the full locale of the current language.
  */
  const CLocale& GetLocale() const;

  /*!
   \brief Returns the system's current locale.
   */
  const std::locale& GetSystemLocale() const { return m_systemLocale; }

  bool ForceUnicodeFont() const { return m_forceUnicodeFont; }

  const std::string& GetDateFormat(bool bLongDate = false) const;
  void SetDateFormat(const std::string& dateFormat, bool bLongDate = false);
  const std::string& GetShortDateFormat() const;
  void SetShortDateFormat(const std::string& shortDateFormat);
  const std::string& GetLongDateFormat() const;
  void SetLongDateFormat(const std::string& longDateFormat);

  const std::string& GetTimeFormat() const;
  void SetTimeFormat(const std::string& timeFormat);
  bool Use24HourClock() const;
  void Set24HourClock(bool use24HourClock);
  void Set24HourClock(const std::string& str24HourClock);
  const std::string& GetMeridiemSymbol(MeridiemSymbol symbol) const;
  static const std::string& MeridiemSymbolToString(MeridiemSymbol symbol);

  CTemperature::Unit GetTemperatureUnit() const;
  void SetTemperatureUnit(CTemperature::Unit temperatureUnit);
  void SetTemperatureUnit(const std::string& temperatureUnit);
  const std::string& GetTemperatureUnitString() const;
  static const std::string& GetTemperatureUnitString(CTemperature::Unit temperatureUnit);
  std::string GetTemperatureAsString(const CTemperature& temperature) const;

  CSpeed::Unit GetSpeedUnit() const;
  void SetSpeedUnit(CSpeed::Unit speedUnit);
  void SetSpeedUnit(const std::string& speedUnit);
  const std::string& GetSpeedUnitString() const;
  static const std::string& GetSpeedUnitString(CSpeed::Unit speedUnit);
  std::string GetSpeedAsString(const CSpeed& speed) const;

  void GetRegionNames(std::vector<std::string>& array);
  void SetCurrentRegion(const std::string& strName);
  const std::string& GetCurrentRegion() const;

  std::set<std::string> GetSortTokens() const;

  static std::string GetLanguagePath() { return "resource://"; }
  static std::string GetLanguagePath(const std::string &language);
  static std::string GetLanguageInfoPath(const std::string &language);

  static void LoadTokens(const TiXmlNode* pTokens, std::set<std::string>& vecTokens);

  static void SettingOptionsLanguageNamesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsStreamLanguagesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsISO6391LanguagesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsRegionsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsShortDateFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsLongDateFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsTimeFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptions24HourClockFormatsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsTemperatureUnitsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsSpeedUnitsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);

protected:
  void SetDefaults();

  static bool DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat);
  static bool DetermineUseMeridiemFromTimeFormat(const std::string& timeFormat);
  static std::string PrepareTimeFormat(const std::string& timeFormat, bool use24HourClock);

  class CRegion
  {
  public:
    CRegion(const CRegion& region);
    CRegion();
    virtual ~CRegion();
    void SetDefaults();
    void SetTemperatureUnit(const std::string& strUnit);
    void SetSpeedUnit(const std::string& strUnit);
    void SetTimeZone(const std::string& strTimeZone);
    void SetGlobalLocale();
    std::string m_strLangLocaleName;
    std::string m_strLangLocaleCodeTwoChar;
    std::string m_strRegionLocaleName;
    std::string m_strName;
    std::string m_strDateFormatLong;
    std::string m_strDateFormatShort;
    std::string m_strTimeFormat;
    std::string m_strMeridiemSymbols[2];
    std::string m_strTimeZone;

    CTemperature::Unit m_tempUnit;
    CSpeed::Unit m_speedUnit;
  };


  typedef std::map<std::string, CRegion> MAPREGIONS;
  typedef std::map<std::string, CRegion>::iterator ITMAPREGIONS;
  typedef std::pair<std::string, CRegion> PAIR_REGIONS;
  MAPREGIONS m_regions;
  CRegion* m_currentRegion; // points to the current region
  CRegion m_defaultRegion; // default, will be used if no region available via langinfo.xml
  std::locale m_systemLocale;     // current locale, matching GUI settings

  LanguageResourcePtr m_languageAddon;

  std::string m_strGuiCharSet;
  bool m_forceUnicodeFont;
  std::string m_strSubtitleCharSet;
  std::string m_strDVDMenuLanguage;
  std::string m_strDVDAudioLanguage;
  std::string m_strDVDSubtitleLanguage;
  std::set<std::string> m_sortTokens;

  std::string m_shortDateFormat;
  std::string m_longDateFormat;
  std::string m_timeFormat;
  bool m_use24HourClock;
  CTemperature::Unit m_temperatureUnit;
  CSpeed::Unit m_speedUnit;

  std::string m_audioLanguage;
  std::string m_subtitleLanguage;
  // this is the general (not win32-specific) three char language code
  std::string m_languageCodeGeneral;
};


XBMC_GLOBAL_REF(CLangInfo, g_langInfo);
#define g_langInfo XBMC_GLOBAL_USE(CLangInfo)
