#include "include.h"
#include "LocalizeStrings.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "XMLUtils.h"

// can be removed after hardcoded strings are in strings.xml
CStdString ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.IsEmpty())
    return str;

  CStdString ret;
  g_charsetConverter.stringCharsetToUtf8(strEncoding, str, ret);
  return ret;
}

CLocalizeStrings g_localizeStrings;
extern CStdString g_LoadErrorStr;

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

bool CLocalizeStrings::Load(const CStdString& strFileName)
{
  m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
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
  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    const TiXmlNode *pChildID = pChild->FirstChild("id");
    const TiXmlNode *pChildText = pChild->FirstChild("value");
    if (pChildID && !pChildID->NoChildren() &&
        pChildText && !pChildText->NoChildren())
    { // Load old style language file with nodes for id and value
      DWORD dwID = atoi(pChildID->FirstChild()->Value());

      CStdString utf8String;
      if (strEncoding.IsEmpty()) // Is language file utf8?
        utf8String=pChildText->FirstChild()->Value();
      else
        g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);

      m_vecStrings[dwID] = utf8String;
    }
    else
    { // Load new style language file with id as attribute
      const char* attrId=pChild->Attribute("id");
      if (attrId && !pChild->NoChildren())
      {
        DWORD dwID = atoi(attrId);
        CStdString utf8String;
        if (strEncoding.IsEmpty()) // Is language file utf8?
          utf8String=pChild->FirstChild()->Value();
        else
          g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);

        m_vecStrings[dwID] = utf8String;
      }
    }
    pChild = pChild->NextSiblingElement("string");
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

    const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
    while (pChild)
    {
      const TiXmlNode *pChildID = pChild->FirstChild("id");
      const TiXmlNode *pChildText = pChild->FirstChild("value");
      if (pChildID && !pChildID->NoChildren() &&
          pChildText && !pChildText->NoChildren())
      { // Load old style language file with nodes for id and value
        DWORD dwID = atoi(pChildID->FirstChild()->Value());

        ivecStrings i = m_vecStrings.find(dwID);
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
      else
      { // Load new style language file with id as attribute
        const char* attrId=pChild->Attribute("id");
        if (attrId && !pChild->NoChildren())
        {
          DWORD dwID = atoi(attrId);
          ivecStrings i = m_vecStrings.find(dwID);
          if (i == m_vecStrings.end())
          {
            CStdString utf8String;
            if (strEncoding.IsEmpty()) // Is language file utf8?
              utf8String=pChild->FirstChild()->Value();
            else
              g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8String);

            m_vecStrings[dwID] = utf8String;
          }
        }
      }
      pChild = pChild->NextSiblingElement("string");
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
  m_vecStrings[20027] = ToUTF8(strEncoding, "°F");
  m_vecStrings[20028] = ToUTF8(strEncoding, "K");
  m_vecStrings[20029] = ToUTF8(strEncoding, "°C");
  m_vecStrings[20030] = ToUTF8(strEncoding, "°Ré");
  m_vecStrings[20031] = ToUTF8(strEncoding, "°Ra"); 
  m_vecStrings[20032] = ToUTF8(strEncoding, "°Rø"); 
  m_vecStrings[20033] = ToUTF8(strEncoding, "°De"); 
  m_vecStrings[20034] = ToUTF8(strEncoding, "°N");
  m_vecStrings[20035] = ToUTF8(strEncoding, "km/h");
  m_vecStrings[20036] = ToUTF8(strEncoding, "mph");
  m_vecStrings[20037] = ToUTF8(strEncoding, "m/s");
  m_vecStrings[20038] = "Lock music";
  m_vecStrings[20039] = "Lock videos";
  m_vecStrings[20040] = "Lock pictures";
  m_vecStrings[20041] = "Lock programs and scripts";
  m_vecStrings[20042] = "Lock files";
  m_vecStrings[20043] = "Lock settings";
  m_vecStrings[20044] = "Use individual locks per share";
  m_vecStrings[20045] = "Enter master mode";
  m_vecStrings[20046] = "Leave master mode";
  m_vecStrings[20047] = "Are your sure you want to change share locks mode?";
  m_vecStrings[20048] = "This will remove all current share locks.";
  m_vecStrings[20049] = "Best Available";
  m_vecStrings[20050] = "Autoswitch between 16x9 and 4x3";
  m_vecStrings[20051] = "Treat stacked files as single file";
  m_vecStrings[20052] = "Caution";
  m_vecStrings[20053] = "Left master mode";
  m_vecStrings[20054] = "Entered master mode";
  m_vecStrings[20055] = "Allmusic.com Thumb";
  m_vecStrings[20056] = "Source Thumbnail";
  m_vecStrings[20057] = "Remove Thumbnail";
  m_vecStrings[20058] = "Enter the new value";
  m_vecStrings[20059] = "Query Info For All Albums";

  // new strings for weather localization
  m_vecStrings[1411] = "with";
  m_vecStrings[1412] = "windy";

  return true;
}

static CStdString szEmptyString = "";

const CStdString& CLocalizeStrings::Get(DWORD dwCode) const
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
