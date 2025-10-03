/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/GlobalsHandling.h"
#include "utils/Locale.h"
#include "utils/Speed.h"
#include "utils/Temperature.h"

#include <locale>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#ifdef TARGET_WINDOWS
#ifdef GetDateFormat
#undef GetDateFormat
#endif // GetDateFormat
#ifdef GetTimeFormat
#undef GetTimeFormat
#endif // GetTimeFormat
#endif // TARGET_WINDOWS

class TiXmlNode;
struct StringSettingOption;

namespace ADDON
{
  class CLanguageResource;
}
typedef std::shared_ptr<ADDON::CLanguageResource> LanguageResourcePtr;

enum class MeridiemSymbol
{
  PM = 0,
  AM
};

class CLangInfo : public ISettingCallback, public ISettingsHandler
{
public:
  CLangInfo();
  ~CLangInfo() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // implementation of ISettingsHandler
  void OnSettingsLoaded() override;

  /*
   * \brief Get language codes list of the installed language addons.
   * \param languages [OUT] The list of languages (language code, name).
   */
  static void GetAddonsLanguageCodes(std::map<std::string, std::string>& languages);

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
  // The 2 char language code if it exists, else the 3 char code.
  const std::string& GetLanguageISO6391() const { return m_languageISO6391; }
  // The 3 char language code.
  const std::string& GetLanguageISO6392() const { return m_languageISO6392; }
  // The ISO name of the language (Not the description in the config file).
  const std::string& GetLanguageISOEnglishName() const { return m_languageISOEnglishName; }

  /*!
   * \brief Convert an english language name to an addon locale,
   *        by searching in the installed language addons.
   * \param langName [IN] The english language name
   * \return The locale for the given english name, or empty if not found
   */
  static std::string ConvertEnglishNameToAddonLocale(const std::string& langName);

  /*!
   * \brief Get the english language name from given locale,
   *        by searching in the installed language addons.
   * \param locale [OPT] Locale of the language (current if empty)
   */
  std::string GetEnglishLanguageName(const std::string& locale = "") const;

  /*!
  \brief Sets and loads the given (or configured) language, its details and strings.

  \param strLanguage (optional) Language to be loaded.
  \param reloadServices (optional) Whether to reload services relying on localization.
  \return True if the language has been successfully loaded, false otherwise.
  */
  bool SetLanguage(std::string strLanguage = "", bool reloadServices = true);

  /*
   * \brief Get the audio language in ISO 639-2 format.
   * \param allowFallback If set to true, when audio language setting is set to "default", "original" or "mediadefault"
   *                      the returned value can fallback to a general language code (e.g. eng), otherwise an empty value is returned.
   * \return The language code (user-defined also allowed). The value can be empty when allowFallback if set to false.
   */
  const std::string& GetAudioLanguage(bool allowFallback) const;

  /*
   * \brief Set the audio language.
   * \param language The language can either be a two char language code,
   *        or a three char language code, or a language name in english,
   *        also user-defined languages are allowed.
   * \param isIso6392 Defines that language is in ISO 639-2 format, otherwise will be considered as ISO 639-1 format.
   */
  void SetAudioLanguage(const std::string& language, bool isIso6392 = false);

  /*
   * \brief Get the subtitle language in ISO 639-2 format.
   * \param allowFallback If set to true, when audio language setting is set to "default", "original"
   *                      the returned value can fallback to a general language code (e.g. eng), otherwise an empty value is returned.
   * \return The language code (user-defined also allowed). The value can be empty when allowFallback if set to false.
   */
  const std::string& GetSubtitleLanguage(bool allowFallback) const;

  /*
   * \brief Set the subtitle language.
   * \param language The language can either be a two char language code,
   *        or a three char language code, or a language name in english,
   *        also user-defined languages are allowed.
   * \param isIso6392 Defines that language is in ISO 639-2 format, otherwise will be considered as ISO 639-1 format.
   */
  void SetSubtitleLanguage(const std::string& language, bool isIso6392 = false);

  const std::string GetDVDMenuLanguage() const;
  const std::string GetDVDAudioLanguage() const;
  const std::string GetDVDSubtitleLanguage() const;
  const std::string& GetTimeZone() const;

  const std::string& GetRegionLocale() const;

  const std::locale& GetOriginalLocale() const;

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
  const std::string& GetCurrentRegionISO31661Alpha2() const;
  const std::string& GetCurrentRegionISO31661Alpha3() const;
  const std::string& GetCurrentRegionISO31661EnglishName() const;

  using Tokens = std::set<std::string, std::less<>>;
  Tokens GetSortTokens() const;

  static std::string GetLanguagePath() { return "resource://"; }
  static std::string GetLanguagePath(const std::string &language);
  static std::string GetLanguageInfoPath(const std::string &language);
  bool UseLocaleCollation();

  static void LoadTokens(const TiXmlNode* pTokens, Tokens& vecTokens);

  static void SettingOptionsLanguageNamesFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<StringSettingOption>& list,
                                                std::string& current);
  static void SettingOptionsAudioStreamLanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);
  static void SettingOptionsSubtitleStreamLanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);
  static void SettingOptionsSubtitleDownloadlanguagesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current);
  static void SettingOptionsISO6391LanguagesFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current);
  static void SettingOptionsRegionsFiller(const std::shared_ptr<const CSetting>& setting,
                                          std::vector<StringSettingOption>& list,
                                          std::string& current);
  static void SettingOptionsShortDateFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current);
  static void SettingOptionsLongDateFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                  std::vector<StringSettingOption>& list,
                                                  std::string& current);
  static void SettingOptionsTimeFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                              std::vector<StringSettingOption>& list,
                                              std::string& current);
  static void SettingOptions24HourClockFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                     std::vector<StringSettingOption>& list,
                                                     std::string& current);
  static void SettingOptionsTemperatureUnitsFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current);
  static void SettingOptionsSpeedUnitsFiller(const std::shared_ptr<const CSetting>& setting,
                                             std::vector<StringSettingOption>& list,
                                             std::string& current);

protected:
  void SetDefaults();
  bool Load(const std::string& strLanguage);

  static bool DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat);
  static bool DetermineUseMeridiemFromTimeFormat(const std::string& timeFormat);
  static std::string PrepareTimeFormat(const std::string& timeFormat, bool use24HourClock);
  static void AddLanguages(std::vector<StringSettingOption> &list);

  class CRegion final
  {
  public:
    CRegion();
    void SetDefaults();
    void SetTemperatureUnit(const std::string& strUnit);
    void SetSpeedUnit(const std::string& strUnit);
    void SetTimeZone(const std::string& strTimeZone);

    class custom_numpunct : public std::numpunct<char>
    {
    public:
      custom_numpunct(const char decimal_point, const char thousands_sep, const std::string& grouping)
        : cDecimalPoint(decimal_point), cThousandsSep(thousands_sep), sGroup(grouping) {}
    protected:
      char do_decimal_point() const override { return cDecimalPoint; }
      char do_thousands_sep() const override { return cThousandsSep; }
      std::string do_grouping() const override { return sGroup; }
    private:
      const char cDecimalPoint;
      const char cThousandsSep;
      const std::string sGroup;
    };

    /*! \brief Set the locale associated with this region global.

    Set the locale associated with this region global. This affects string
    sorting & transformations.
    */
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
    std::string m_strGrouping;
    char m_cDecimalSep;
    char m_cThousandsSep;

    CTemperature::Unit m_tempUnit;
    CSpeed::Unit m_speedUnit;
    std::string m_regionISO31661Alpha2;
    std::string m_regionISO31661Alpha3;
    std::string m_regionISO31661EnglishName;
  };

  typedef std::map<std::string, CRegion> MAPREGIONS;
  typedef std::map<std::string, CRegion>::iterator ITMAPREGIONS;
  typedef std::pair<std::string, CRegion> PAIR_REGIONS;
  MAPREGIONS m_regions;
  CRegion* m_currentRegion; // points to the current region
  CRegion m_defaultRegion; // default, will be used if no region available via langinfo.xml
  std::locale m_systemLocale;     // current locale, matching GUI settings
  std::locale m_originalLocale; // original locale, without changes of collate
  int m_collationtype;
  LanguageResourcePtr m_languageAddon;

  std::string m_strGuiCharSet;
  bool m_forceUnicodeFont;
  std::string m_strSubtitleCharSet;
  std::string m_strDVDMenuLanguage;
  std::string m_strDVDAudioLanguage;
  std::string m_strDVDSubtitleLanguage;
  Tokens m_sortTokens;

  std::string m_shortDateFormat;
  std::string m_longDateFormat;
  std::string m_timeFormat;
  bool m_use24HourClock;
  CTemperature::Unit m_temperatureUnit;
  CSpeed::Unit m_speedUnit;

  std::string m_audioLanguage; // ISO 639-2 three char (not win32 specific)
  std::string m_subtitleLanguage; // ISO 639-2 three char (not win32 specific)
  std::string m_languageCodeGeneral; // ISO 639-2 three char (not win32-specific)
  std::string m_languageISO6391; // 2 char ISO ISO 639-1 (3 char if no 2 char available)
  std::string m_languageISO6392; // 3 char ISO ISO 639-2
  std::string m_languageISOEnglishName; // The ISO english name of this language
};


XBMC_GLOBAL_REF(CLangInfo, g_langInfo);
#define g_langInfo XBMC_GLOBAL_USE(CLangInfo)
