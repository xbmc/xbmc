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

  typedef enum {
    MERIDIEM_SYMBOL_PM=0,
    MERIDIEM_SYMBOL_AM
  } MERIDIEM_SYMBOL;

  CStdString GetDateFormat(bool bLongDate=false) const;
  CStdString GetTimeFormat() const;
  CStdString GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const;
  void GetRegionNames(CStdStringArray& array);
  void SetCurrentRegion(const CStdString& strName);
  CStdString GetCurrentRegion();

protected:
  void Clear();

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
    CStdString strMeridiemSymbols[2];
  } REGION, *LPREGION;

  typedef map<CStdString, REGION> MAPREGIONS;
  typedef map<CStdString, REGION>::iterator ITMAPREGIONS;
  typedef pair<CStdString, REGION> PAIR_REGIONS;
  MAPREGIONS m_regions;
  LPREGION m_currentRegion;
};


extern CLangInfo g_langInfo;
