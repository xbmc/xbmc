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
