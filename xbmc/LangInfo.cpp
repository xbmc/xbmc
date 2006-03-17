#include "stdafx.h"
#include "LangInfo.h"

CLangInfo g_langInfo;

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
}

CStdString CLangInfo::GetGuiCharSet() const
{
  CStdString strCharSet;
  //strCharSet=g_guiSettings.GetString("LookAndFeel.CharSet");
  //if (strCharSet=="DEFAULT")
  {
    strCharSet=m_strGuiCharSet;
    if (strCharSet.IsEmpty())
      strCharSet="CP1252";
  }

  return strCharSet;
}

CStdString CLangInfo::GetSubtitleCharSet() const
{
  CStdString strCharSet=g_guiSettings.GetString("Subtitles.CharSet");
  if (strCharSet=="DEFAULT")
  {
    strCharSet=m_strSubtitleCharSet;
    if (strCharSet.IsEmpty())
      strCharSet="CP1252";
  }

  return strCharSet;
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDMenuLanguage() const
{
  return (!m_strDVDMenuLanguage.IsEmpty() ? m_strDVDMenuLanguage : "en");
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDAudioLanguage() const
{
  return (!m_strDVDAudioLanguage.IsEmpty() ? m_strDVDAudioLanguage : "en");
}

// two character codes as defined in ISO639
CStdString CLangInfo::GetDVDSubtitleLanguage() const
{
  return (!m_strDVDSubtitleLanguage.IsEmpty() ? m_strDVDSubtitleLanguage : "en");
}
