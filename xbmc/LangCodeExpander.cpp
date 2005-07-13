#include "stdafx.h"
#include "langcodeexpander.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CLangCodeExpander g_LangCodeExpander;

CLangCodeExpander::CLangCodeExpander(void)
{}

CLangCodeExpander::~CLangCodeExpander(void)
{
  Clear();
}
void CLangCodeExpander::LoadUserCodes(const TiXmlElement* pRootElement)
{
  if (pRootElement)
    LoadCodes(pRootElement, m_mapUser);
}

void CLangCodeExpander::LoadStandardCodes(void)
{
  try
  {
    //    if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
    //   {
    LoadCodes("Q:\\system\\players\\mplayer\\ISO639-1.xml", m_mapISO639_1);
    LoadCodes("Q:\\system\\players\\mplayer\\ISO639-2.xml", m_mapISO639_2);
    /*    }
        else
        {
          LoadCodes("Q:\\mplayer\\ISO639-1.xml", m_mapISO639_1);
          LoadCodes("Q:\\mplayer\\ISO639-2.xml", m_mapISO639_2);
        }*/
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "ERROR: loading language translation codes. No codes will be loaded");
    ClearMap(m_mapISO639_1);
    ClearMap(m_mapISO639_2);
  }
}

bool CLangCodeExpander::Lookup(CStdString& desc, const CStdString& code, CLangCodeExpander::ELangCodeTypes type)
{
  int iSplit = code.find("-");
  if (iSplit > 0)
  {
    CStdString strLeft, strRight;
    const bool bLeft = Lookup(strLeft, code.Left(iSplit), type);
    const bool bRight = Lookup(strRight, code.Mid(iSplit + 1), type);
    if (bLeft || bRight)
    {
      desc = "";
      if (strLeft.length() > 0)
        desc = strLeft;
      else
        desc = code.Left(iSplit);

      if (strRight.length() > 0)
      {
        desc += " - ";
        desc += strRight;
      }
      else
      {
        desc += " - ";
        desc += code.Mid(iSplit + 1);
      }
      return true;
    }
    return false;
  }
  else
  {
    if (type == ELT_USER)
      return LookupInMap(desc, code, m_mapUser);
    else if (type == ELT_ISO639_1)
      return LookupInMap(desc, code, m_mapISO639_1);
    else if (type == ELT_ISO639_2)
      return LookupInMap(desc, code, m_mapISO639_1);
    else
    {
      if (LookupInMap(desc, code, m_mapUser)) return true;

      if (code.length() == 2)
        if (LookupInMap(desc, code, m_mapISO639_1)) return true;

      if (code.length() == 3)
        if (LookupInMap(desc, code, m_mapISO639_2)) return true;
    }
  }
  return false;
}

bool CLangCodeExpander::LookupDVDLangCode(CStdString& desc, const int code)
{

  char lang[3];
  lang[2] = 0;
  lang[1] = (code & 255);
  lang[0] = (code >> 8) & 255;

  return Lookup(desc, lang, ELT_ISO639_1);
}


void CLangCodeExpander::LoadCodes(const TiXmlElement* pRootElement, CLangCodeExpander::STRINGLOOKUPTABLE& m_map)
{
  ClearMap(m_map);

  int sLen, lLen;
  char* sShort, * sLong;

  TiXmlNode* pLangCode = pRootElement->FirstChild("code");
  while (pLangCode)
  {
    TiXmlNode* pShort = pLangCode->FirstChildElement("short");
    TiXmlNode* pLong = pLangCode->FirstChildElement("long");
    if (pShort && pLong)
    {
      //Only use one allocation, might gain something on that
      sLen = strlen(pShort->FirstChild()->Value());
      lLen = strlen(pLong->FirstChild()->Value());

      sShort = new char[sLen + lLen + 2 ];
      sLong = &(sShort[sLen + 1]);

      strcpy(sShort, pShort->FirstChild()->Value());
      strcpy(sLong, pLong->FirstChild()->Value());
      strlwr(sShort); //Lowercase all keys for lookup
      CStdString strDesc;
      if (!LookupInMap(strDesc, sShort, m_map))
        m_map.insert(
          STRINGLOOKUPTABLE::value_type(
            sShort,
            sLong));
      else
        delete[] sShort;
    }
    pLangCode = pLangCode->NextSibling();
  }
}

void CLangCodeExpander::LoadCodes(const CStdString& strXMLFile, CLangCodeExpander::STRINGLOOKUPTABLE& m_map)
{

  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) )
  {
    if (xmlDoc.ErrorRow() > 0)
      CLog::Log(LOGERROR, "%s, Line %d\n%s", strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    else
      CLog::Log(LOGERROR, "unable to load file %s", strXMLFile.c_str());

    return ;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if ( strValue != "languagecodes")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <languagecodes>", strXMLFile.c_str());
    return ;
  }

  LoadCodes(pRootElement, m_map);
  xmlDoc.Clear();
  CLog::Log(LOGNOTICE, "loaded %s", strXMLFile.c_str());
}


inline bool CLangCodeExpander::LookupInMap(CStdString& desc, const CStdString& code, CLangCodeExpander::STRINGLOOKUPTABLE& slmap)
{
  STRINGLOOKUPTABLE::iterator it;
  //Make sure we convert to lowercase before trying to find it
  CStdString sCode(code);
  sCode.MakeLower();
  sCode.TrimLeft();
  sCode.TrimRight();
  it = slmap.find(sCode.c_str());
  if (it != slmap.end())
  {
    desc = it->second;
    return true;
  }
  return false;
}

inline void CLangCodeExpander::ClearMap(CLangCodeExpander::STRINGLOOKUPTABLE& slmap)
{
  STRINGLOOKUPTABLE::iterator it;
  it = slmap.begin();
  while (it != slmap.end())
  {
    delete [] it->first;
    it++;
  }
  slmap.clear();
  return ;

}

void CLangCodeExpander::Clear()
{
  ClearMap(m_mapISO639_1);
  ClearMap(m_mapISO639_2);
  ClearMap(m_mapUser);
}
