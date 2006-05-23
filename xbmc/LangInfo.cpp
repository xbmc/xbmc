#include "stdafx.h"
#include "LangInfo.h"

CLangInfo g_langInfo;

#define TEMP_UNIT_STRINGS 20027

#define SPEED_UNIT_STRINGS 20035

CLangInfo::CLangInfo()
{
  Clear();
}

CLangInfo::~CLangInfo()
{
}

bool CLangInfo::Load(const CStdString& strFileName)
{
  Clear();

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strFileName.c_str()))
  {
    CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("language"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <language>", strFileName.c_str());
    return false;
  }

  const TiXmlNode *pCharSets = pRootElement->FirstChild("charsets");
  if (pCharSets && !pCharSets->NoChildren())
  {
    const TiXmlNode *pGui = pCharSets->FirstChild("gui");
    if (pGui && !pGui->NoChildren())
    {
      CStdString strForceUnicodeFont = ((TiXmlElement*) pGui)->Attribute("unicodefont");

      if (strForceUnicodeFont.Equals("true"))
        m_forceUnicodeFont=true;

      m_strGuiCharSet=pGui->FirstChild()->Value();
    }

    const TiXmlNode *pSubtitle = pCharSets->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_strSubtitleCharSet=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pDVD = pRootElement->FirstChild("dvd");
  if (pDVD && !pDVD->NoChildren())
  {
    const TiXmlNode *pMenu = pDVD->FirstChild("menu");
    if (pMenu && !pMenu->NoChildren())
      m_strDVDMenuLanguage=pMenu->FirstChild()->Value();

    const TiXmlNode *pAudio = pDVD->FirstChild("audio");
    if (pAudio && !pAudio->NoChildren())
      m_strDVDAudioLanguage=pAudio->FirstChild()->Value();

    const TiXmlNode *pSubtitle = pDVD->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_strDVDSubtitleLanguage=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pRegions = pRootElement->FirstChild("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const TiXmlElement *pRegion=pRegions->FirstChildElement("region");
    while (pRegion)
    {
      REGION region;
      region.strName=pRegion->Attribute("name");
      if (region.strName.IsEmpty())
        region.strName="N/A";

      const TiXmlNode *pDateLong=pRegion->FirstChild("datelong");
      if (pDateLong && !pDateLong->NoChildren())
        region.strDateFormatLong=pDateLong->FirstChild()->Value();

      const TiXmlNode *pDateShort=pRegion->FirstChild("dateshort");
      if (pDateShort && !pDateShort->NoChildren())
        region.strDateFormatShort=pDateShort->FirstChild()->Value();

      const TiXmlElement *pTime=pRegion->FirstChildElement("time");
      if (pTime && !pTime->NoChildren())
      {
        region.strTimeFormat=pTime->FirstChild()->Value();
        region.strMeridiemSymbols[MERIDIEM_SYMBOL_AM]=pTime->Attribute("symbolAM");
        region.strMeridiemSymbols[MERIDIEM_SYMBOL_PM]=pTime->Attribute("symbolPM");
      }

      m_regions.insert(PAIR_REGIONS(region.strName, region));

      const TiXmlNode *pTempUnit=pRegion->FirstChild("tempunit");
      if (pTempUnit && !pTempUnit->NoChildren())
        SetTempUnit(pTempUnit->FirstChild()->Value());

      const TiXmlNode *pSpeedUnit=pRegion->FirstChild("speedunit");
      if (pSpeedUnit && !pSpeedUnit->NoChildren())
        SetSpeedUnit(pSpeedUnit->FirstChild()->Value());

      pRegion=pRegion->NextSiblingElement("region");
    }

    const CStdString& strName=g_guiSettings.GetString("XBDateTime.Region");
    SetCurrentRegion(strName);
  }
  return true;
}

void CLangInfo::Clear()
{
  m_strGuiCharSet.Empty();
  m_strSubtitleCharSet.Empty();
  m_strDVDMenuLanguage.Empty();
  m_strDVDAudioLanguage.Empty();
  m_strDVDSubtitleLanguage.Empty();
  m_forceUnicodeFont=false;
  m_regions.clear();
  m_currentRegion=NULL;

  m_strGuiCharSet="CP1252";
  m_strSubtitleCharSet="CP1252";
  m_strDVDMenuLanguage="en";
  m_strDVDAudioLanguage="en";
  m_strDVDSubtitleLanguage="en";

  m_tempUnit=TEMP_UNIT_CELSIUS;
  m_speedUnit=SPEED_UNIT_KMH;
}

CStdString CLangInfo::GetGuiCharSet() const
{
  CStdString strCharSet;
  strCharSet=g_guiSettings.GetString("LookAndFeel.CharSet");
  if (strCharSet=="DEFAULT")
    strCharSet=m_strGuiCharSet;

  return strCharSet;
}

CStdString CLangInfo::GetSubtitleCharSet() const
{
  CStdString strCharSet=g_guiSettings.GetString("Subtitles.CharSet");
  if (strCharSet=="DEFAULT")
    strCharSet=m_strSubtitleCharSet;

  return strCharSet;
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDMenuLanguage() const
{
  return m_strDVDMenuLanguage;
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDAudioLanguage() const
{
  return m_strDVDAudioLanguage;
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDSubtitleLanguage() const
{
  return m_strDVDSubtitleLanguage;
}

// Returns the format string for the date of the current language
CStdString CLangInfo::GetDateFormat(bool bLongDate/*=false*/) const
{
  if (bLongDate)
    return (m_currentRegion ? m_currentRegion->strDateFormatLong : "DDDD, D. MMMM YYYY");
  else
    return (m_currentRegion ? m_currentRegion->strDateFormatShort : "DD/MM/YYYY");
}

// Returns the format string for the time of the current language
CStdString CLangInfo::GetTimeFormat() const
{
  return (m_currentRegion ? m_currentRegion->strTimeFormat : "HH:mm:ss");
}

// Returns the AM/PM symbol of the current language
CStdString CLangInfo::GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const
{
  return (m_currentRegion ? m_currentRegion->strMeridiemSymbols[symbol] : "");
}

// Fills the array with the region names available for this language
void CLangInfo::GetRegionNames(CStdStringArray& array)
{
  for (ITMAPREGIONS it=m_regions.begin(); it!=m_regions.end(); ++it)
  {
    CStdString strName=it->first;
    if (strName=="N/A")
      strName=g_localizeStrings.Get(416);
    array.push_back(strName);
  }
}

// Set the current region by its name, names from GetRegionNames() are valid.
// If the region is not found the first available region is set.
void CLangInfo::SetCurrentRegion(const CStdString& strName)
{
  ITMAPREGIONS it=m_regions.find(strName);
  if (it!=m_regions.end())
    m_currentRegion=&it->second;
  else
    m_currentRegion=&m_regions.begin()->second;
}

// Returns the current region set for this language
CStdString CLangInfo::GetCurrentRegion()
{
  return (m_currentRegion ? m_currentRegion->strName : "");
}

// Returns the temperature unit string for the current language
const CStdString& CLangInfo::GetTempUnitString()
{
  return g_localizeStrings.Get(TEMP_UNIT_STRINGS+m_tempUnit);
}

void CLangInfo::SetTempUnit(const CStdString& strUnit)
{
  if (strUnit.Equals("F"))
    m_tempUnit=TEMP_UNIT_FAHRENHEIT;
  else if (strUnit.Equals("K"))
    m_tempUnit=TEMP_UNIT_KELVIN;
  else if (strUnit.Equals("C"))
    m_tempUnit=TEMP_UNIT_CELSIUS;
  else if (strUnit.Equals("Re"))
    m_tempUnit=TEMP_UNIT_REAUMUR;
  else if (strUnit.Equals("Ra"))
    m_tempUnit=TEMP_UNIT_RANKINE;
  else if (strUnit.Equals("Ro"))
    m_tempUnit=TEMP_UNIT_ROMER;
  else if (strUnit.Equals("De"))
    m_tempUnit=TEMP_UNIT_DELISLE;
  else if (strUnit.Equals("N"))
    m_tempUnit=TEMP_UNIT_NEWTON;
}

// Returns the speed unit string for the current language
const CStdString& CLangInfo::GetSpeedUnitString()
{
  return g_localizeStrings.Get(SPEED_UNIT_STRINGS+m_speedUnit);
}

void CLangInfo::SetSpeedUnit(const CStdString& strUnit)
{
  if (strUnit.Equals("kmh"))
    m_speedUnit=SPEED_UNIT_KMH;
  else if (strUnit.Equals("mph"))
    m_speedUnit=SPEED_UNIT_MPH;
  else if (strUnit.Equals("mps"))
    m_speedUnit=SPEED_UNIT_MPS;
}
