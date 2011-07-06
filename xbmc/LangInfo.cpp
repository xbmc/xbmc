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
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"
#ifdef _WIN32
#include "utils/LangCodeExpander.h"
#endif

using namespace std;

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
  m_strLangLocaleName = region.m_strLangLocaleName;
  m_strRegionLocaleName = region.m_strRegionLocaleName;

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
  m_strLangLocaleName = "English";

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

// set the locale associated with this region global. This affects string
// sorting & transformations
void CLangInfo::CRegion::SetGlobalLocale()
{
  CStdString strLocale;
  if (m_strRegionLocaleName.length() > 0)
  {
    strLocale = m_strLangLocaleName + "_" + m_strRegionLocaleName;
#ifdef _LINUX
    strLocale += ".UTF-8";
#endif
  }

  CLog::Log(LOGDEBUG, "trying to set locale to %s", strLocale.c_str());

  // We need to set the locale to only change the collate. Otherwise,
  // decimal separator is changed depending of the current language
  // (ie. "," in French or Dutch instead of "."). This breaks atof() and
  // others similar functions.
#if defined(__FreeBSD__)
  // on FreeBSD libstdc++ is compiled with "generic" locale support
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
    locale lcl = locale(strLocale);
    strLocale = lcl.name();
    current_locale = current_locale.combine< collate<wchar_t> >(lcl);

    assert(use_facet< numpunct<char> >(current_locale).decimal_point() == '.');

  } catch(...) {
    current_locale = locale::classic();
    strLocale = "C";
  }

  locale::global(current_locale);
#endif
  CLog::Log(LOGINFO, "global locale set to %s", strLocale.c_str());
}

CLangInfo::CLangInfo()
{
  SetDefaults();
  SetIsoLangMap();
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

  if (pRootElement->Attribute("locale"))
    m_defaultRegion.m_strLangLocaleName = pRootElement->Attribute("locale");

#ifdef _WIN32
  // Windows need 3 chars isolang code
  if (m_defaultRegion.m_strLangLocaleName.length() == 2)
  {
    if (! g_LangCodeExpander.ConvertTwoToThreeCharCode(m_defaultRegion.m_strLangLocaleName, m_defaultRegion.m_strLangLocaleName, true))
      m_defaultRegion.m_strLangLocaleName = "";
  }
#endif

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

      if (pRegion->Attribute("locale"))
        region.m_strRegionLocaleName = pRegion->Attribute("locale");

#ifdef _WIN32
      // Windows need 3 chars regions code
      if (region.m_strRegionLocaleName.length() == 2)
      {
        if (! g_LangCodeExpander.ConvertLinuxToWindowsRegionCodes(region.m_strRegionLocaleName, region.m_strRegionLocaleName))
          region.m_strRegionLocaleName = "";
      }
#endif

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

void CLangInfo::SetIsoLangMap()
{
  m_isoLangMap["aar"] = "aa";
  m_isoLangMap["abk"] = "ab";
  m_isoLangMap["ave"] = "ae";
  m_isoLangMap["afr"] = "af";
  m_isoLangMap["aka"] = "ak";
  m_isoLangMap["amh"] = "am";
  m_isoLangMap["arg"] = "an";
  m_isoLangMap["ara"] = "ar";
  m_isoLangMap["asm"] = "as";
  m_isoLangMap["ava"] = "av";
  m_isoLangMap["aym"] = "ay";
  m_isoLangMap["aze"] = "az";
  m_isoLangMap["bak"] = "ba";
  m_isoLangMap["bel"] = "be";
  m_isoLangMap["bul"] = "bg";
  m_isoLangMap["bih"] = "bh";
  m_isoLangMap["bis"] = "bi";
  m_isoLangMap["bam"] = "bm";
  m_isoLangMap["ben"] = "bn";
  m_isoLangMap["bod"] = "bo";
  m_isoLangMap["tib"] = "bo"; // Alternate.
  m_isoLangMap["bre"] = "br";
  m_isoLangMap["bos"] = "bs";
  m_isoLangMap["cat"] = "ca";
  m_isoLangMap["che"] = "ce";
  m_isoLangMap["cha"] = "ch";
  m_isoLangMap["cos"] = "co";
  m_isoLangMap["cre"] = "cr";
  m_isoLangMap["ces"] = "cs";
  m_isoLangMap["cze"] = "cs";
  m_isoLangMap["chu"] = "cu";
  m_isoLangMap["chv"] = "cv";
  m_isoLangMap["cym"] = "cy";
  m_isoLangMap["wel"] = "cy"; // Alternate.
  m_isoLangMap["dan"] = "da";
  m_isoLangMap["deu"] = "de";
  m_isoLangMap["ger"] = "de"; // Alternate.
  m_isoLangMap["div"] = "dv";
  m_isoLangMap["dzo"] = "dz";
  m_isoLangMap["ewe"] = "ee";
  m_isoLangMap["ell"] = "el";
  m_isoLangMap["gre"] = "el"; // Alternate.
  m_isoLangMap["eng"] = "en";
  m_isoLangMap["epo"] = "eo";
  m_isoLangMap["spa"] = "es";
  m_isoLangMap["est"] = "et";
  m_isoLangMap["eus"] = "eu";
  m_isoLangMap["baq"] = "eu"; // Alternate.
  m_isoLangMap["fas"] = "fa";
  m_isoLangMap["per"] = "fa"; // Alternate.
  m_isoLangMap["ful"] = "ff";
  m_isoLangMap["fin"] = "fi";
  m_isoLangMap["fij"] = "fj";
  m_isoLangMap["fao"] = "fo";
  m_isoLangMap["fra"] = "fr";
  m_isoLangMap["fre"] = "fr"; // Alternate.
  m_isoLangMap["fry"] = "fy";
  m_isoLangMap["gle"] = "ga";
  m_isoLangMap["gla"] = "gd";
  m_isoLangMap["glg"] = "gl";
  m_isoLangMap["grn"] = "gn";
  m_isoLangMap["guj"] = "gu";
  m_isoLangMap["glv"] = "gv";
  m_isoLangMap["hau"] = "ha";
  m_isoLangMap["heb"] = "he";
  m_isoLangMap["hin"] = "hi";
  m_isoLangMap["hmo"] = "ho";
  m_isoLangMap["hrv"] = "hr";
  m_isoLangMap["hat"] = "ht";
  m_isoLangMap["hun"] = "hu";
  m_isoLangMap["hye"] = "hy";
  m_isoLangMap["arm"] = "hy"; // Alternate.
  m_isoLangMap["her"] = "hz";
  m_isoLangMap["ina"] = "ia";
  m_isoLangMap["ind"] = "id";
  m_isoLangMap["ile"] = "ie";
  m_isoLangMap["ibo"] = "ig";
  m_isoLangMap["iii"] = "ii";
  m_isoLangMap["ipk"] = "ik";
  m_isoLangMap["ido"] = "io";
  m_isoLangMap["isl"] = "is";
  m_isoLangMap["ice"] = "is";
  m_isoLangMap["ita"] = "it";
  m_isoLangMap["iku"] = "iu";
  m_isoLangMap["jpn"] = "ja";
  m_isoLangMap["jav"] = "jv";
  m_isoLangMap["kat"] = "ka";
  m_isoLangMap["geo"] = "ka";
  m_isoLangMap["kon"] = "kg";
  m_isoLangMap["kik"] = "ki";
  m_isoLangMap["kua"] = "kj";
  m_isoLangMap["kaz"] = "kk";
  m_isoLangMap["kal"] = "kl";
  m_isoLangMap["khm"] = "km";
  m_isoLangMap["kan"] = "kn";
  m_isoLangMap["kor"] = "ko";
  m_isoLangMap["kau"] = "kr";
  m_isoLangMap["kas"] = "ks";
  m_isoLangMap["kur"] = "ku";
  m_isoLangMap["kom"] = "kv";
  m_isoLangMap["cor"] = "kw";
  m_isoLangMap["kir"] = "ky";
  m_isoLangMap["lat"] = "la";
  m_isoLangMap["ltz"] = "lb";
  m_isoLangMap["lug"] = "lg";
  m_isoLangMap["lim"] = "li";
  m_isoLangMap["lin"] = "ln";
  m_isoLangMap["lao"] = "lo";
  m_isoLangMap["lit"] = "lt";
  m_isoLangMap["lub"] = "lu";
  m_isoLangMap["lav"] = "lv";
  m_isoLangMap["mlg"] = "mg";
  m_isoLangMap["mah"] = "mh";
  m_isoLangMap["mri"] = "mi";
  m_isoLangMap["mao"] = "mi"; // Alternate.
  m_isoLangMap["mkd"] = "mk";
  m_isoLangMap["mac"] = "mk"; // Alternate.
  m_isoLangMap["mal"] = "ml";
  m_isoLangMap["mon"] = "mn";
  m_isoLangMap["mar"] = "mr";
  m_isoLangMap["msa"] = "ms";
  m_isoLangMap["may"] = "ms"; // Alternate.
  m_isoLangMap["mlt"] = "mt";
  m_isoLangMap["mya"] = "my";
  m_isoLangMap["bur"] = "my"; // Alternate.
  m_isoLangMap["nau"] = "na";
  m_isoLangMap["nob"] = "nb";
  m_isoLangMap["nde"] = "nd";
  m_isoLangMap["nep"] = "ne";
  m_isoLangMap["ndo"] = "ng";
  m_isoLangMap["nld"] = "nl";
  m_isoLangMap["dut"] = "nl"; // Alternate.
  m_isoLangMap["nno"] = "nn";
  m_isoLangMap["nor"] = "no";
  m_isoLangMap["nbl"] = "nr";
  m_isoLangMap["nav"] = "nv";
  m_isoLangMap["nya"] = "ny";
  m_isoLangMap["oci"] = "oc";
  m_isoLangMap["oji"] = "oj";
  m_isoLangMap["orm"] = "om";
  m_isoLangMap["ori"] = "or";
  m_isoLangMap["oss"] = "os";
  m_isoLangMap["pan"] = "pa";
  m_isoLangMap["pli"] = "pi";
  m_isoLangMap["pol"] = "pl";
  m_isoLangMap["pus"] = "ps";
  m_isoLangMap["por"] = "pt";
  m_isoLangMap["que"] = "qu";
  m_isoLangMap["roh"] = "rm";
  m_isoLangMap["run"] = "rn";
  m_isoLangMap["ron"] = "ro";
  m_isoLangMap["rum"] = "ro"; // Alternate.
  m_isoLangMap["rus"] = "ru";
  m_isoLangMap["kin"] = "rw";
  m_isoLangMap["san"] = "sa";
  m_isoLangMap["srd"] = "sc";
  m_isoLangMap["snd"] = "sd";
  m_isoLangMap["sme"] = "se";
  m_isoLangMap["sag"] = "sg";
  m_isoLangMap["sin"] = "si";
  m_isoLangMap["slk"] = "sk";
  m_isoLangMap["slo"] = "sk"; // Alternate.
  m_isoLangMap["slv"] = "sl";
  m_isoLangMap["smo"] = "sm";
  m_isoLangMap["sna"] = "sn";
  m_isoLangMap["som"] = "so";
  m_isoLangMap["sqi"] = "sq";
  m_isoLangMap["srp"] = "sr";
  m_isoLangMap["ssw"] = "ss";
  m_isoLangMap["sot"] = "st";
  m_isoLangMap["sun"] = "su";
  m_isoLangMap["swe"] = "sv";
  m_isoLangMap["swa"] = "sw";
  m_isoLangMap["tam"] = "ta";
  m_isoLangMap["tel"] = "te";
  m_isoLangMap["tgk"] = "tg";
  m_isoLangMap["tha"] = "th";
  m_isoLangMap["tir"] = "ti";
  m_isoLangMap["tuk"] = "tk";
  m_isoLangMap["tgl"] = "tl";
  m_isoLangMap["tsn"] = "tn";
  m_isoLangMap["ton"] = "to";
  m_isoLangMap["tur"] = "tr";
  m_isoLangMap["tso"] = "ts";
  m_isoLangMap["tat"] = "tt";
  m_isoLangMap["twi"] = "tw";
  m_isoLangMap["tah"] = "ty";
  m_isoLangMap["uig"] = "ug";
  m_isoLangMap["ukr"] = "uk";
  m_isoLangMap["urd"] = "ur";
  m_isoLangMap["uzb"] = "uz";
  m_isoLangMap["ven"] = "ve";
  m_isoLangMap["vie"] = "vi";
  m_isoLangMap["vol"] = "vo";
  m_isoLangMap["wln"] = "wa";
  m_isoLangMap["wol"] = "wo";
  m_isoLangMap["xho"] = "xh";
  m_isoLangMap["yid"] = "yi";
  m_isoLangMap["yor"] = "yo";
  m_isoLangMap["zha"] = "za";
  m_isoLangMap["zho"] = "zh";
  m_isoLangMap["chi"] = "zh"; // Alternate.
  m_isoLangMap["zul"] = "zu";
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

const CStdString& CLangInfo::GetLanguageLocale() const
{
  return m_currentRegion->m_strLangLocaleName;
}

const CStdString& CLangInfo::GetRegionLocale() const
{
  return m_currentRegion->m_strRegionLocaleName;
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

  m_currentRegion->SetGlobalLocale();
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

CStdString CLangInfo::ConvertIso6392ToIso6391(const CStdString& language)
{
  ITMAPSTRINGS it = m_isoLangMap.find(language);
  if (it != m_isoLangMap.end())
  {
    CStdString converted(it->second);
    return converted;
  }

  return "";
}
