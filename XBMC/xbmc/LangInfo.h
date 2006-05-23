#pragma once

class CLangInfo
{
public:
  CLangInfo();
  virtual ~CLangInfo();

  bool Load(const CStdString& strFileName);

  CStdString GetGuiCharSet() const;
  CStdString GetSubtitleCharSet() const;

  CStdString GetDVDMenuLanguage() const;
  CStdString GetDVDAudioLanguage() const;
  CStdString GetDVDSubtitleLanguage() const;

  bool ForceUnicodeFont() { return m_forceUnicodeFont; }

  CStdString GetDateFormat(bool bLongDate=false) const;
  
  typedef enum {
    MERIDIEM_SYMBOL_PM=0,
    MERIDIEM_SYMBOL_AM,
    MERIDIEM_SYMBOL_MAX
  } MERIDIEM_SYMBOL;

  CStdString GetTimeFormat() const;
  CStdString GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const;

  typedef enum {
    TEMP_UNIT_FAHRENHEIT=0,
    TEMP_UNIT_KELVIN,
    TEMP_UNIT_CELSIUS,
    TEMP_UNIT_REAUMUR,
    TEMP_UNIT_RANKINE,
    TEMP_UNIT_ROMER,
    TEMP_UNIT_DELISLE,
    TEMP_UNIT_NEWTON
  } TEMP_UNIT;

  const CStdString& GetTempUnitString();
  CLangInfo::TEMP_UNIT GetTempUnit() { return m_tempUnit; }

  typedef enum _SPEED_UNIT
  {
    SPEED_UNIT_KMH=0,
    SPEED_UNIT_MPH,
    SPEED_UNIT_MPS
  } SPEED_UNIT;

  const CStdString& GetSpeedUnitString();
  CLangInfo::SPEED_UNIT GetSpeedUnit() { return m_speedUnit; }

  void GetRegionNames(CStdStringArray& array);
  void SetCurrentRegion(const CStdString& strName);
  CStdString GetCurrentRegion();

protected:
  void Clear();
  void SetTempUnit(const CStdString& strUnit);
  void SetSpeedUnit(const CStdString& strUnit);

protected:
  CStdString m_strGuiCharSet;
  CStdString m_strSubtitleCharSet;
  CStdString m_strDVDMenuLanguage;
  CStdString m_strDVDAudioLanguage;
  CStdString m_strDVDSubtitleLanguage;
  bool m_forceUnicodeFont;

  typedef struct _REGION
  {
    CStdString strName;
    CStdString strDateFormatLong;
    CStdString strDateFormatShort;
    CStdString strTimeFormat;
    CStdString strMeridiemSymbols[MERIDIEM_SYMBOL_MAX];
  } REGION, *LPREGION;

  typedef map<CStdString, REGION> MAPREGIONS;
  typedef map<CStdString, REGION>::iterator ITMAPREGIONS;
  typedef pair<CStdString, REGION> PAIR_REGIONS;
  MAPREGIONS m_regions;
  LPREGION m_currentRegion;

  TEMP_UNIT m_tempUnit;
  SPEED_UNIT m_speedUnit;
};


extern CLangInfo g_langInfo;
