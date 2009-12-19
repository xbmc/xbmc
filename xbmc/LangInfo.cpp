/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "LangInfo.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"

using namespace std;

CLangInfo g_langInfo;

#define TEMP_UNIT_STRINGS 20027

#define SPEED_UNIT_STRINGS 20200

CLangInfo::CRegion::CRegion(const CRegion& region)
{
  m_strName=region.m_strName;
  m_forceUnicodeFont=region.m_forceUnicodeFont;
  m_strGuiCharSet=region.m_strGuiCharSet;
  m_strSubtitleCharSet=region.m_strSubtitleCharSet;
  m_strDVDMenuLanguage=region.m_strDVDMenuLanguage;
  m_strDVDAudioLanguage=region.m_strDVDAudioLanguage;
  m_strDVDSubtitleLanguage=region.m_strDVDSubtitleLanguage;

  m_strDateFormatShort=region.m_strDateFormatShort;
  m_strDateFormatLong=region.m_strDateFormatLong;
  m_strTimeFormat=region.m_strTimeFormat;
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM];
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM];
  m_strTimeFormat=region.m_strTimeFormat;
  m_tempUnit=region.m_tempUnit;
  m_speedUnit=region.m_speedUnit;
  m_strTimeZone = region.m_strTimeZone;
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
  m_forceUnicodeFont=false;
  m_strGuiCharSet="CP1252";
  m_strSubtitleCharSet="CP1252";
  m_strDVDMenuLanguage="en";
  m_strDVDAudioLanguage="en";
  m_strDVDSubtitleLanguage="en";

  m_strDateFormatShort="DD/MM/YYYY";
  m_strDateFormatLong="DDDD, D MMMM YYYY";
  m_strTimeFormat="HH:mm:ss";
  m_tempUnit=TEMP_UNIT_CELSIUS;
  m_speedUnit=SPEED_UNIT_KMH;
  m_strTimeZone.clear();
}

void CLangInfo::CRegion::SetTempUnit(const CStdString& strUnit)
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

void CLangInfo::CRegion::SetSpeedUnit(const CStdString& strUnit)
{
  if (strUnit.Equals("kmh"))
    m_speedUnit=SPEED_UNIT_KMH;
  else if (strUnit.Equals("mpmin"))
    m_speedUnit=SPEED_UNIT_MPMIN;
  else if (strUnit.Equals("mps"))
    m_speedUnit=SPEED_UNIT_MPS;
  else if (strUnit.Equals("fth"))
    m_speedUnit=SPEED_UNIT_FTH;
  else if (strUnit.Equals("ftm"))
    m_speedUnit=SPEED_UNIT_FTMIN;
  else if (strUnit.Equals("fts"))
    m_speedUnit=SPEED_UNIT_FTS;
  else if (strUnit.Equals("mph"))
    m_speedUnit=SPEED_UNIT_MPH;
  else if (strUnit.Equals("kts"))
    m_speedUnit=SPEED_UNIT_KTS;
  else if (strUnit.Equals("beaufort"))
    m_speedUnit=SPEED_UNIT_BEAUFORT;
  else if (strUnit.Equals("inchs"))
    m_speedUnit=SPEED_UNIT_INCHPS;
  else if (strUnit.Equals("yards"))
    m_speedUnit=SPEED_UNIT_YARDPS;
  else if (strUnit.Equals("fpf"))
    m_speedUnit=SPEED_UNIT_FPF;
}

void CLangInfo::CRegion::SetTimeZone(const CStdString& strTimeZone)
{
  m_strTimeZone = strTimeZone;
}

CLangInfo::CLangInfo()
{
  SetDefaults();
}

CLangInfo::~CLangInfo()
{
}

bool CLangInfo::Load(const CStdString& strFileName)
{
  SetDefaults();

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strFileName))
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
        m_defaultRegion.m_forceUnicodeFont=true;

      m_defaultRegion.m_strGuiCharSet=pGui->FirstChild()->Value();
    }

    const TiXmlNode *pSubtitle = pCharSets->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_defaultRegion.m_strSubtitleCharSet=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pDVD = pRootElement->FirstChild("dvd");
  if (pDVD && !pDVD->NoChildren())
  {
    const TiXmlNode *pMenu = pDVD->FirstChild("menu");
    if (pMenu && !pMenu->NoChildren())
      m_defaultRegion.m_strDVDMenuLanguage=pMenu->FirstChild()->Value();

    const TiXmlNode *pAudio = pDVD->FirstChild("audio");
    if (pAudio && !pAudio->NoChildren())
      m_defaultRegion.m_strDVDAudioLanguage=pAudio->FirstChild()->Value();

    const TiXmlNode *pSubtitle = pDVD->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_defaultRegion.m_strDVDSubtitleLanguage=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pRegions = pRootElement->FirstChild("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const TiXmlElement *pRegion=pRegions->FirstChildElement("region");
    while (pRegion)
    {
      CRegion region(m_defaultRegion);
      region.m_strName=pRegion->Attribute("name");
      if (region.m_strName.IsEmpty())
        region.m_strName="N/A";

      const TiXmlNode *pDateLong=pRegion->FirstChild("datelong");
      if (pDateLong && !pDateLong->NoChildren())
        region.m_strDateFormatLong=pDateLong->FirstChild()->Value();

      const TiXmlNode *pDateShort=pRegion->FirstChild("dateshort");
      if (pDateShort && !pDateShort->NoChildren())
        region.m_strDateFormatShort=pDateShort->FirstChild()->Value();

      const TiXmlElement *pTime=pRegion->FirstChildElement("time");
      if (pTime && !pTime->NoChildren())
      {
        region.m_strTimeFormat=pTime->FirstChild()->Value();
        region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM]=pTime->Attribute("symbolAM");
        region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM]=pTime->Attribute("symbolPM");
      }

      const TiXmlNode *pTempUnit=pRegion->FirstChild("tempunit");
      if (pTempUnit && !pTempUnit->NoChildren())
        region.SetTempUnit(pTempUnit->FirstChild()->Value());

      const TiXmlNode *pSpeedUnit=pRegion->FirstChild("speedunit");
      if (pSpeedUnit && !pSpeedUnit->NoChildren())
        region.SetSpeedUnit(pSpeedUnit->FirstChild()->Value());

      const TiXmlNode *pTimeZone=pRegion->FirstChild("timezone");
      if (pTimeZone && !pTimeZone->NoChildren())
        region.SetTimeZone(pTimeZone->FirstChild()->Value());

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion=pRegion->NextSiblingElement("region");
    }

    const CStdString& strName=g_guiSettings.GetString("locale.country");
    SetCurrentRegion(strName);
  }

  LoadTokens(pRootElement->FirstChild("sorttokens"),g_advancedSettings.m_vecTokens);

  return true;
}

void CLangInfo::LoadTokens(const TiXmlNode* pTokens, vector<CStdString>& vecTokens)
{
  if (pTokens && !pTokens->NoChildren())
  {
    const TiXmlElement *pToken = pTokens->FirstChildElement("token");
    while (pToken)
    {
      CStdString strSep= " ._";
      if (pToken->Attribute("separators"))
        strSep = pToken->Attribute("separators");
      if (pToken->FirstChild() && pToken->FirstChild()->Value())
      {
        if (strSep.IsEmpty())
          vecTokens.push_back(pToken->FirstChild()->Value());
        else
          for (unsigned int i=0;i<strSep.size();++i)
            vecTokens.push_back(CStdString(pToken->FirstChild()->Value())+strSep[i]);
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
  m_currentRegion=&m_defaultRegion;
}

CStdString CLangInfo::GetGuiCharSet() const
{
  CStdString strCharSet;
  strCharSet=g_guiSettings.GetString("locale.charset");
  if (strCharSet=="DEFAULT")
    strCharSet=m_currentRegion->m_strGuiCharSet;

  return strCharSet;
}

CStdString CLangInfo::GetSubtitleCharSet() const
{
  CStdString strCharSet=g_guiSettings.GetString("subtitles.charset");
  if (strCharSet=="DEFAULT")
    strCharSet=m_currentRegion->m_strSubtitleCharSet;

  return strCharSet;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDMenuLanguage() const
{
  return m_currentRegion->m_strDVDMenuLanguage;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDAudioLanguage() const
{
  return m_currentRegion->m_strDVDAudioLanguage;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDSubtitleLanguage() const
{
  return m_currentRegion->m_strDVDSubtitleLanguage;
}

// Returns the format string for the date of the current language
const CStdString& CLangInfo::GetDateFormat(bool bLongDate/*=false*/) const
{
  if (bLongDate)
    return m_currentRegion->m_strDateFormatLong;
  else
    return m_currentRegion->m_strDateFormatShort;
}

// Returns the format string for the time of the current language
const CStdString& CLangInfo::GetTimeFormat() const
{
  return m_currentRegion->m_strTimeFormat;
}

const CStdString& CLangInfo::GetTimeZone() const
{
  return m_currentRegion->m_strTimeZone;
}

// Returns the AM/PM symbol of the current language
const CStdString& CLangInfo::GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const
{
  return m_currentRegion->m_strMeridiemSymbols[symbol];
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
  else if (!m_regions.empty())
    m_currentRegion=&m_regions.begin()->second;
  else
    m_currentRegion=&m_defaultRegion;
}

// Returns the current region set for this language
const CStdString& CLangInfo::GetCurrentRegion() const
{
  return m_currentRegion->m_strName;
}

CLangInfo::TEMP_UNIT CLangInfo::GetTempUnit() const
{
  return m_currentRegion->m_tempUnit;
}

// Returns the temperature unit string for the current language
const CStdString& CLangInfo::GetTempUnitString() const
{
  return g_localizeStrings.Get(TEMP_UNIT_STRINGS+m_currentRegion->m_tempUnit);
}

CLangInfo::SPEED_UNIT CLangInfo::GetSpeedUnit() const
{
  return m_currentRegion->m_speedUnit;
}

// Returns the speed unit string for the current language
const CStdString& CLangInfo::GetSpeedUnitString() const
{
  return g_localizeStrings.Get(SPEED_UNIT_STRINGS+m_currentRegion->m_speedUnit);
}
