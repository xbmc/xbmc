#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/StdString.h"

#include <map>

class TiXmlNode;

class CLangInfo
{
public:
  CLangInfo();
  virtual ~CLangInfo();

  bool Load(const CStdString& strFileName);

  CStdString GetGuiCharSet() const;
  CStdString GetSubtitleCharSet() const;

  // three char language code (not win32 specific)
  const CStdString& GetLanguageCode() const { return m_languageCodeGeneral; }

  const CStdString& GetAudioLanguage() const;
  // language can either be a two char language code as defined in ISO639
  // or a three char language code
  // or a language name in english (as used by XBMC)
  void SetAudioLanguage(const CStdString &language);
  
  // three char language code (not win32 specific)
  const CStdString& GetSubtitleLanguage() const;
  // language can either be a two char language code as defined in ISO639
  // or a three char language code
  // or a language name in english (as used by XBMC)
  void SetSubtitleLanguage(const CStdString &language);

  const CStdString& GetDVDMenuLanguage() const;
  const CStdString& GetDVDAudioLanguage() const;
  const CStdString& GetDVDSubtitleLanguage() const;
  const CStdString& GetTimeZone() const;

  const CStdString& GetRegionLocale() const;
  const CStdString& GetLanguageLocale() const;

  bool ForceUnicodeFont() const { return m_currentRegion->m_forceUnicodeFont; }

  const CStdString& GetDateFormat(bool bLongDate=false) const;

  typedef enum _MERIDIEM_SYMBOL
  {
    MERIDIEM_SYMBOL_PM=0,
    MERIDIEM_SYMBOL_AM,
    MERIDIEM_SYMBOL_MAX
  } MERIDIEM_SYMBOL;

  const CStdString& GetTimeFormat() const;
  const CStdString& GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const;

  typedef enum _TEMP_UNIT
  {
    TEMP_UNIT_FAHRENHEIT=0,
    TEMP_UNIT_KELVIN,
    TEMP_UNIT_CELSIUS,
    TEMP_UNIT_REAUMUR,
    TEMP_UNIT_RANKINE,
    TEMP_UNIT_ROMER,
    TEMP_UNIT_DELISLE,
    TEMP_UNIT_NEWTON
  } TEMP_UNIT;

  const CStdString& GetTempUnitString() const;
  CLangInfo::TEMP_UNIT GetTempUnit() const;


  typedef enum _SPEED_UNIT
  {
    SPEED_UNIT_KMH=0, // kilemetre per hour
    SPEED_UNIT_MPMIN, // metres per minute
    SPEED_UNIT_MPS, // metres per second
    SPEED_UNIT_FTH, // feet per hour
    SPEED_UNIT_FTMIN, // feet per minute
    SPEED_UNIT_FTS, // feet per second
    SPEED_UNIT_MPH, // miles per hour
    SPEED_UNIT_KTS, // knots
    SPEED_UNIT_BEAUFORT, // beaufort
    SPEED_UNIT_INCHPS, // inch per second
    SPEED_UNIT_YARDPS, // yard per second
    SPEED_UNIT_FPF // Furlong per Fortnight
  } SPEED_UNIT;

  const CStdString& GetSpeedUnitString() const;
  CLangInfo::SPEED_UNIT GetSpeedUnit() const;

  void GetRegionNames(CStdStringArray& array);
  void SetCurrentRegion(const CStdString& strName);
  const CStdString& GetCurrentRegion() const;

  static void LoadTokens(const TiXmlNode* pTokens, std::vector<CStdString>& vecTokens);
protected:
  void SetDefaults();

protected:

  class CRegion
  {
  public:
    CRegion(const CRegion& region);
    CRegion();
    virtual ~CRegion();
    void SetDefaults();
    void SetTempUnit(const CStdString& strUnit);
    void SetSpeedUnit(const CStdString& strUnit);
    void SetTimeZone(const CStdString& strTimeZone);
    void SetGlobalLocale();
    CStdString m_strGuiCharSet;
    CStdString m_strSubtitleCharSet;
    CStdString m_strDVDMenuLanguage;
    CStdString m_strDVDAudioLanguage;
    CStdString m_strDVDSubtitleLanguage;
    CStdString m_strLangLocaleName;
    CStdString m_strRegionLocaleName;
    bool m_forceUnicodeFont;
    CStdString m_strName;
    CStdString m_strDateFormatLong;
    CStdString m_strDateFormatShort;
    CStdString m_strTimeFormat;
    CStdString m_strMeridiemSymbols[MERIDIEM_SYMBOL_MAX];
    CStdString m_strTimeZone;

    TEMP_UNIT m_tempUnit;
    SPEED_UNIT m_speedUnit;
  };


  typedef std::map<CStdString, CRegion> MAPREGIONS;
  typedef std::map<CStdString, CRegion>::iterator ITMAPREGIONS;
  typedef std::pair<CStdString, CRegion> PAIR_REGIONS;
  MAPREGIONS m_regions;
  CRegion* m_currentRegion; // points to the current region
  CRegion m_defaultRegion; // default, will be used if no region available via langinfo.xml

  CStdString m_audioLanguage;
  CStdString m_subtitleLanguage;
  // this is the general (not win32-specific) three char language code
  CStdString m_languageCodeGeneral;
};


extern CLangInfo g_langInfo;
