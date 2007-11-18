#include "include.h"
#include "LocalizeStrings.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/Util.h"
#include "XMLUtils.h"

CLocalizeStrings g_localizeStrings;
CLocalizeStrings g_localizeStringsTemp;
extern CStdString g_LoadErrorStr;

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

CStdString CLocalizeStrings::ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.IsEmpty())
    return str;

  CStdString ret;
  g_charsetConverter.stringCharsetToUtf8(strEncoding, str, ret);
  return ret;
}

void CLocalizeStrings::ClearSkinStrings()
{
  // clear the skin strings
  unsigned int skin_strings_start = 31001;
  unsigned int skin_strings_end = 32000;
  for (unsigned int str = skin_strings_start; str < skin_strings_end; str++)
  {
    iStrings it = m_strings.find(str);
    if (it != m_strings.end())
      m_strings.erase(it);
  }
}

bool CLocalizeStrings::LoadSkinStrings(const CStdString& path, const CStdString& fallbackPath)
{
  ClearSkinStrings();
  // load the skin strings in.
  CStdString encoding, error;
  if (!LoadXML(path, encoding, error))
  {
    if (path == fallbackPath) // no fallback, nothing to do
      return false;
  }

  // load the fallback
  if (path != fallbackPath)
    LoadXML(fallbackPath, encoding, error);

  return true;
}

bool CLocalizeStrings::LoadXML(const CStdString &filename, CStdString &encoding, CStdString &error)
{
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(filename.c_str()))
  {
    CLog::Log(LOGERROR, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    error.Format("Unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  XMLUtils::GetEncoding(&xmlDoc, encoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || 
       pRootElement->ValueStr()!=CStdString("strings"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <strings>", filename.c_str());
    error.Format("%s\nDoesnt start with <strings>", filename.c_str());
    return false;
  }

  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    // Load new style language file with id as attribute
    const char* attrId=pChild->Attribute("id");
    if (attrId && !pChild->NoChildren())
    {
      DWORD dwID = atoi(attrId);
      if (m_strings.find(dwID) == m_strings.end())
        m_strings[dwID] = ToUTF8(encoding, pChild->FirstChild()->Value());
    }
    pChild = pChild->NextSiblingElement("string");
  }
  return true;
}

bool CLocalizeStrings::Load(const CStdString& strFileName, const CStdString& strFallbackFileName)
{
  bool bLoadFallback = !strFileName.Equals(strFallbackFileName);

  CStdString encoding, error;
  Clear();

  if (!LoadXML(strFileName, encoding, error))
  {
    // try loading the fallback
    if (!bLoadFallback || !LoadXML(strFallbackFileName, encoding, error))
    {
      g_LoadErrorStr = error;
      return false;
    }
    bLoadFallback = false;
  }

  if (bLoadFallback)
    LoadXML(strFallbackFileName, encoding, error);

  // fill in the constant strings
  m_strings[20022] = "";
  m_strings[20027] = ToUTF8(encoding, "°F");
  m_strings[20028] = ToUTF8(encoding, "K");
  m_strings[20029] = ToUTF8(encoding, "°C");
  m_strings[20030] = ToUTF8(encoding, "°Ré");
  m_strings[20031] = ToUTF8(encoding, "°Ra"); 
  m_strings[20032] = ToUTF8(encoding, "°Rø"); 
  m_strings[20033] = ToUTF8(encoding, "°De"); 
  m_strings[20034] = ToUTF8(encoding, "°N");

  m_strings[20200] = ToUTF8(encoding, "km/h");
  m_strings[20201] = ToUTF8(encoding, "m/min");
  m_strings[20202] = ToUTF8(encoding, "m/s");
  m_strings[20203] = ToUTF8(encoding, "ft/h");
  m_strings[20204] = ToUTF8(encoding, "ft/min");
  m_strings[20205] = ToUTF8(encoding, "ft/s");
  m_strings[20206] = ToUTF8(encoding, "mph");
  m_strings[20207] = ToUTF8(encoding, "kts");
  m_strings[20208] = ToUTF8(encoding, "Beaufort");
  m_strings[20209] = ToUTF8(encoding, "inch/s");
  m_strings[20210] = ToUTF8(encoding, "yard/s");
  m_strings[20211] = ToUTF8(encoding, "Furlong/Fortnight");

  return true;
}

static CStdString szEmptyString = "";

const CStdString& CLocalizeStrings::Get(DWORD dwCode) const
{
  ciStrings i = m_strings.find(dwCode);
  if (i == m_strings.end())
  {
    return szEmptyString;
  }
  return i->second;
}

void CLocalizeStrings::Clear()
{
  m_strings.clear();
}
