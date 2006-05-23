#include "include.h"
#include "LocalizeStrings.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "XMLUtils.h"


CLocalizeStrings g_localizeStrings;
extern CStdString g_LoadErrorStr;

CLocalizeStrings::CLocalizeStrings(void)
{}

CLocalizeStrings::~CLocalizeStrings(void)
{}


bool CLocalizeStrings::Load(const CStdString& strFileName)
{
  m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
  {
    TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(strFileName.c_str()))
    {
      CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      g_LoadErrorStr.Format("%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
      return false;
    }
    
    CStdString strEncoding;
    XMLUtils::GetEncoding(&xmlDoc, strEncoding);

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue != CStdString("strings"))
    {
      CLog::Log(LOGERROR, "%s Doesn't contain <strings>", strFileName.c_str());
      g_LoadErrorStr.Format("%s\nDoesnt start with <strings>", strFileName.c_str());
      return false;
    }
    const TiXmlNode *pChild = pRootElement->FirstChild();
    while (pChild)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "string")
      {
        const TiXmlNode *pChildID = pChild->FirstChild("id");
        const TiXmlNode *pChildText = pChild->FirstChild("value");
        DWORD dwID = atoi(pChildID->FirstChild()->Value());
        CStdString utf8String;
        if (!pChildText->NoChildren())
        {
          if (strEncoding.IsEmpty()) // Is language file utf8?
            utf8String=pChildText->FirstChild()->Value();
          else
            g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);
        }

        if (!utf8String.IsEmpty())
          m_vecStrings[dwID] = utf8String;
      }
      pChild = pChild->NextSibling();
    }
  }

  if (!strFileName.Equals("Q:\\language\\english\\strings.xml"))
  {
    // load the original english file
    // and copy any missing texts
    TiXmlDocument xmlDoc;
    if ( !xmlDoc.LoadFile("Q:\\language\\english\\strings.xml") )
    {
      return true;
    }

    CStdString strEncoding;
    XMLUtils::GetEncoding(&xmlDoc, strEncoding);

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue != CStdString("strings")) return false;
    const TiXmlNode *pChild = pRootElement->FirstChild();
    while (pChild)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "string")
      {
        const TiXmlNode *pChildID = pChild->FirstChild("id");
        const TiXmlNode *pChildText = pChild->FirstChild("value");
        if (pChildID->FirstChild() && pChildText->FirstChild())
        {
          DWORD dwID = atoi(pChildID->FirstChild()->Value());
          ivecStrings i;
          i = m_vecStrings.find(dwID);
          if (i == m_vecStrings.end())
          {
            CStdString utf8String;
            if (strEncoding.IsEmpty()) // Is language file utf8?
              utf8String=pChildText->FirstChild()->Value();
            else
              g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);
            m_vecStrings[dwID] = utf8String;
          }
        }
      }
      pChild = pChild->NextSibling();
    }
  }

  // TODO: localize 2.0
  // Hardcoded strings (starting at 20000)
  m_vecStrings[20000] = "CDDA Rip Folder";
  m_vecStrings[20001] = "Use external DVD Player";
  m_vecStrings[20002] = "External DVD Player";
  m_vecStrings[20003] = "Trainers Folder";
  m_vecStrings[20004] = "Screenshot Folder";
  m_vecStrings[20005] = "Recordings Folder";
  m_vecStrings[20006] = "Playlists Folder";
  m_vecStrings[20007] = "Recordings";
  m_vecStrings[20008] = "Screenshots";
  m_vecStrings[20009] = "Use XBMC";
  m_vecStrings[20010] = "Artist Image";
  m_vecStrings[20011] = "Music Playlists";
  m_vecStrings[20012] = "Video Playlists";
  m_vecStrings[20013] = "Do you wish to launch the game?";
  m_vecStrings[20014] = "Sort by: Playlist";
  m_vecStrings[20015] = "IMDb Thumb";
  m_vecStrings[20016] = "Current Thumb";
  m_vecStrings[20017] = "Local Thumb";
  m_vecStrings[20018] = "No Thumb";
  m_vecStrings[20019] = "Choose Thumbnail";
  m_vecStrings[20020] = "Cannot use both KAI and trainer";
  m_vecStrings[20021] = "Choose which you want to use";
  m_vecStrings[20022] = "";
  m_vecStrings[20023] = "Conflict";
  m_vecStrings[20024] = "Scan new";
  m_vecStrings[20025] = "Scan all"; 
  m_vecStrings[20026] = "Region"; 
  return true;
}

static string szEmptyString = "";

const string& CLocalizeStrings::Get(DWORD dwCode) const
{
  ivecStrings i;
  i = m_vecStrings.find(dwCode);
  if (i == m_vecStrings.end())
  {
    return szEmptyString;
  }
  return i->second;
}

void CLocalizeStrings::Clear()
{
  m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
}
