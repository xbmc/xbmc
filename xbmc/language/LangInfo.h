/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "language/Language.h"
#include "language/LanguageTag.h"
#include "language/Territory.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "utils/GlobalsHandling.h"
#include "utils/Speed.h"
#include "utils/Temperature.h"

#include <locale>
#include <map>
#include <memory>
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

struct StringSettingOption;

namespace KODI::LANGUAGE
{
enum class MeridiemSymbol
{
  PM = 0,
  AM
};

/*!
 * \brief The region profile the user chose, as langinfo.xml states it: date and time formats,
 *        units, number separators and the locale text sorts by.
 */
class CLangInfo : public ISettingCallback, public ISettingsHandler
{
public:
  CLangInfo();
  ~CLangInfo() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // implementation of ISettingsHandler
  void OnSettingsLoaded() override;

  /*!
   * \brief Read the region profiles a language pack ships.
   * \param[in] langInfoPath The path to that pack's langinfo.xml.
   * \return True when the file was read; false leaves the default profile in place.
   */
  bool Load(const std::string& langInfoPath);

  /*!
   * \brief The active language, named in the requested format.
   * \param format The notation to name the language in.
   * \param withRegion Append the active region, separated by "-", in the notation the format
   *        implies: an ISO 3166-1 code for the ISO 639 formats, the region's name otherwise.
   * \return The language, empty when it has no code in the requested format.
   */
  std::string GetLanguageAs(KODI::LANGUAGE::CLanguageTag::Notation format, bool withRegion) const;

  /*!
   * \brief The place the selected region profile is for.
   * \note One accessor rather than one per notation - the caller states which notation it needs,
   *       and the territory answers with nothing where that standard has no code for the place.
   * \return The territory, naming no place where langinfo.xml stated none.
   */
  const KODI::LANGUAGE::CTerritory& GetRegionTerritory() const;

  const std::locale& GetOriginalLocale() const;

  /*!
   \brief Returns the system's current locale.
   */
  const std::locale& GetSystemLocale() const { return m_systemLocale; }

  const std::string& GetDateFormat(bool bLongDate = false) const;
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

  void GetRegionNames(std::vector<std::string>& array) const;
  void SetCurrentRegion(const std::string& strName);
  const std::string& GetCurrentRegion() const;

  bool UseLocaleCollation();

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

  static bool DetermineUse24HourClockFromTimeFormat(const std::string& timeFormat);
  static std::string PrepareTimeFormat(const std::string& timeFormat, bool use24HourClock);

  class CRegion final
  {
  public:
    CRegion();
    void SetDefaults();
    void SetTemperatureUnit(const std::string& strUnit);
    void SetSpeedUnit(const std::string& strUnit);

    class custom_numpunct : public std::numpunct<char>
    {
    public:
      custom_numpunct(const char decimal_point,
                      const char thousands_sep,
                      const std::string& grouping)
        : cDecimalPoint(decimal_point),
          cThousandsSep(thousands_sep),
          sGroup(grouping)
      {
      }

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
    KODI::LANGUAGE::CTerritory m_territory;
    std::string m_strName;
    std::string m_strDateFormatLong;
    std::string m_strDateFormatShort;
    std::string m_strTimeFormat;
    std::string m_strMeridiemSymbols[2];
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
  std::locale m_systemLocale; // current locale, matching GUI settings
  std::locale m_originalLocale; // original locale, without changes of collate
  int m_collationtype;

  std::string m_shortDateFormat;
  std::string m_longDateFormat;
  std::string m_timeFormat;
  bool m_use24HourClock;
  CTemperature::Unit m_temperatureUnit;
  CSpeed::Unit m_speedUnit;
};
} // namespace KODI::LANGUAGE

XBMC_GLOBAL_REF(KODI::LANGUAGE::CLangInfo, g_langInfo);
#define g_langInfo XBMC_GLOBAL_USE(KODI::LANGUAGE::CLangInfo)
