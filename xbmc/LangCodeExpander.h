#pragma once

#include <map>
#include <string>
#include "StdString.h"
#include "tinyxml/tinyxml.h"

class CLangCodeExpander
{
public:

  enum ELangCodeTypes{
    ELT_ANY,
    ELT_ISO639_1,
    ELT_ISO639_2,
    ELT_USER,
  };

  CLangCodeExpander(void);
  ~CLangCodeExpander(void);

  bool Lookup(CStdString& desc, const CStdString& code, ELangCodeTypes type = ELT_ANY);
  bool LookupDVDLangCode(CStdString& desc, const int code);

  void LoadStandardCodes();
  void LoadUserCodes(const TiXmlElement* pRootElement);
protected:


  class str_comp
  {
  public:
    bool operator()(const char* _Left, const char* _Right) const
    {
      return(stricmp(_Left, _Right) < 0);
    }
  };

  typedef std::map<const char *, const char *,str_comp> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapISO639_1;
  STRINGLOOKUPTABLE m_mapISO639_2;
  STRINGLOOKUPTABLE m_mapUser;
  
  inline bool LookupInMap(CStdString& desc, const CStdString& code, STRINGLOOKUPTABLE& slmap);
  inline void ClearMap(STRINGLOOKUPTABLE& slmap);
  
  void LoadCodes(const TiXmlElement* pRootElement, STRINGLOOKUPTABLE& m_map);
  void LoadCodes(const CStdString& strXMLFile, STRINGLOOKUPTABLE& m_map);
};

extern CLangCodeExpander g_LangCodeExpander;
