#include "include.h"
#include "LocalizeStrings.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/Util.h"
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
  if (!pRootElement || pRootElement->NoChildren() || 
       pRootElement->Value()!=CStdString("strings"))
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
          g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8String);

        m_vecStrings[dwID] = utf8String;
      }
    }
    pChild = pChild->NextSiblingElement("string");
  }

  if (!strFileName.Equals(_P("Q:\\language\\english\\strings.xml")))
  {
    // load the original english file
    // and copy any missing texts
    TiXmlDocument xmlDoc;
    if ( !xmlDoc.LoadFile(_P("Q:\\language\\english\\strings.xml")) )
    {
      return true;
    }

    XMLUtils::GetEncoding(&xmlDoc, strEncoding);

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement || pRootElement->NoChildren() || 
         pRootElement->Value()!=CStdString("strings")) return false;

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

  m_vecStrings[20022] = "";
  m_vecStrings[20027] = ToUTF8(strEncoding, "°F");
  m_vecStrings[20028] = ToUTF8(strEncoding, "K");
  m_vecStrings[20029] = ToUTF8(strEncoding, "°C");
  m_vecStrings[20030] = ToUTF8(strEncoding, "°Ré");
  m_vecStrings[20031] = ToUTF8(strEncoding, "°Ra"); 
  m_vecStrings[20032] = ToUTF8(strEncoding, "°Rø"); 
  m_vecStrings[20033] = ToUTF8(strEncoding, "°De"); 
  m_vecStrings[20034] = ToUTF8(strEncoding, "°N");

  m_vecStrings[20200] = ToUTF8(strEncoding, "km/h");
  m_vecStrings[20201] = ToUTF8(strEncoding, "m/min");
  m_vecStrings[20202] = ToUTF8(strEncoding, "m/s");
  m_vecStrings[20203] = ToUTF8(strEncoding, "ft/h");
  m_vecStrings[20204] = ToUTF8(strEncoding, "ft/min");
  m_vecStrings[20205] = ToUTF8(strEncoding, "ft/s");
  m_vecStrings[20206] = ToUTF8(strEncoding, "mph");
  m_vecStrings[20207] = ToUTF8(strEncoding, "kts");
  m_vecStrings[20208] = ToUTF8(strEncoding, "Beaufort");
  m_vecStrings[20209] = ToUTF8(strEncoding, "inch/s");
  m_vecStrings[20210] = ToUTF8(strEncoding, "yard/s");
  m_vecStrings[20211] = ToUTF8(strEncoding, "Furlong/Fortnight");

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
