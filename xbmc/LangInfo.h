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
#include "utils/LangCodeExpander.h"
#include "utils/LanguageTag.h"
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

namespace KODI::LANGINFO
{
// audio language special values
constexpr std::string_view audioLanguageMediaDefault = "mediadefault";
constexpr std::string_view audioLanguageOriginal = "original";
constexpr std::string_view audioLanguageDefault = "default";

// subtitles language special values
constexpr std::string_view subLanguageNone = "none";
constexpr std::string_view subLanguageForcedOnly = "forced_only";
constexpr std::string_view subLanguageOriginal = "original";
constexpr std::string_view subLanguageDefault = "default";
} // namespace KODI::LANGINFO

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
   * \brief The active language, named in the requested format.
   * \param format The notation to name the language in.
   * \param withRegion Append the active region, separated by "-", in the notation the format
   *        implies: an ISO 3166-1 code for the ISO 639 formats, the region's name otherwise.
   * \return The language, empty when it has no code in the requested format.
   */
  std::string GetLanguageAs(CLangCodeExpander::LANGFORMATS format, bool withRegion) const;

  /*!
  \brief Sets and loads the given (or configured) language, its details and strings.

  \param strLanguage (optional) Language to be loaded.
  \param reloadServices (optional) Whether to reload services relying on localization.
  \return True if the language has been successfully loaded, false otherwise.
  */
  bool SetLanguage(std::string strLanguage = "", bool reloadServices = true);

  /*!
   * \brief Get the preferred audio language.
   * \param allowFallback If set to true, when the audio language setting is set to "default",
   *                      "original" or "mediadefault" the UI language is returned instead.
   * \return The language, empty when the setting names no language and allowFallback is false.
   */
  const KODI::UTILS::CLanguageTag& GetAudioLanguage(bool allowFallback) const;

  /*!
   * \brief Set the audio language.
   * \param language The language can either be a two char language code,
   *        or a three char language code, or a language name in english,
   *        also user-defined languages are allowed.
   */
  void SetAudioLanguage(const std::string& language);

  /*!
   * \brief Get the preferred subtitle language.
   * \param allowFallback If set to true, when the subtitle language setting is set to "original"
   *                      or "forced_only" the preferred audio language is returned instead.
   * \return The language, empty when the setting names no language and allowFallback is false.
   */
  const KODI::UTILS::CLanguageTag& GetSubtitleLanguage(bool allowFallback) const;

  /*!
   * \brief Set the subtitle language.
   * \param language The language can either be a two char language code,
   *        or a three char language code, or a language name in english,
   *        also user-defined languages are allowed.
   */
  void SetSubtitleLanguage(const std::string& language);

  KODI::UTILS::CLanguageTag GetDVDMenuLanguage() const;
  KODI::UTILS::CLanguageTag GetDVDAudioLanguage() const;
  KODI::UTILS::CLanguageTag GetDVDSubtitleLanguage() const;
  const std::string& GetTimeZone() const;

  const std::string& GetRegionLocale() const;

  /*!
   * \brief The current region as an ISO 3166-1 alpha-2 code.
   * \return The code, uppercase as ISO 3166-1 publishes it, or an empty string when the region
   *         is not a known one.
   */
  std::string GetRegionCodeAlpha2() const;

  /*!
   * \brief The current region as an ISO 3166-1 alpha-3 code.
   * \return The code, uppercase as ISO 3166-1 publishes it, or an empty string when the region
   *         is not a known one.
   */
  std::string GetRegionCodeAlpha3() const;

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

  void GetRegionNames(std::vector<std::string>& array) const;
  void SetCurrentRegion(const std::string& strName);
  const std::string& GetCurrentRegion() const;

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
                                          std::string& current,
                                          const CLangInfo& langInfo);
  static void SettingOptionsShortDateFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current,
                                                   const CLangInfo& langInfo);
  static void SettingOptionsLongDateFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                  std::vector<StringSettingOption>& list,
                                                  std::string& current,
                                                  const CLangInfo& langInfo);
  static void SettingOptionsTimeFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                              std::vector<StringSettingOption>& list,
                                              std::string& current,
                                              const CLangInfo& langInfo);
  static void SettingOptions24HourClockFormatsFiller(const std::shared_ptr<const CSetting>& setting,
                                                     std::vector<StringSettingOption>& list,
                                                     std::string& current,
                                                     const CLangInfo& langInfo);
  static void SettingOptionsTemperatureUnitsFiller(const std::shared_ptr<const CSetting>& setting,
                                                   std::vector<StringSettingOption>& list,
                                                   std::string& current,
                                                   const CLangInfo& langInfo);
  static void SettingOptionsSpeedUnitsFiller(const std::shared_ptr<const CSetting>& setting,
                                             std::vector<StringSettingOption>& list,
                                             std::string& current,
                                             const CLangInfo& langInfo);

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

    /*!
     * \brief The language and territory this region describes.
     * \note langinfo.xml states them separately and in a form that varies by platform. The
     *       locale is the composed value, normalized, and knows how to render itself.
     */
    CLocale GetLocale() const;

    /*!
     * \brief The region as an ISO 3166-1 alpha-2 code.
     * \note The stored form differs by platform, Windows widening it to alpha-3 when read.
     * \return The code, uppercase as ISO 3166-1 publishes it, or an empty string when the region
     *         is not a known one.
     */
    std::string GetCodeAlpha2() const;

    /*!
     * \brief The region as an ISO 3166-1 alpha-3 code.
     * \return The code, uppercase as ISO 3166-1 publishes it, or an empty string when the region
     *         is not a known one.
     */
    std::string GetCodeAlpha3() const;

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
    void SetGlobalLocale(CLangInfo& langInfo);
    std::string m_strLangLocaleName;
    std::string m_strRegionLocaleName;
    std::string m_strName;
    std::string m_strDateFormatLong;
    std::string m_strDateFormatShort;
    std::string m_strTimeFormat;
    std::string m_strMeridiemSymbols[2];
    std::string m_strTimeZone;
    std::string m_strGrouping;
    char m_cDecimalSep{'.'};
    char m_cThousandsSep{'.'};

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

  KODI::UTILS::CLanguageTag m_audioLanguage;
  KODI::UTILS::CLanguageTag m_subtitleLanguage;
  //! An unset audio or subtitle preference falls back to this
  KODI::UTILS::CLanguageTag m_uiLanguage;
};


XBMC_GLOBAL_REF(CLangInfo, g_langInfo);
#define g_langInfo XBMC_GLOBAL_USE(CLangInfo)
