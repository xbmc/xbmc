#pragma once

class CLangCodeExpander
{
public:

  CLangCodeExpander(void);
  ~CLangCodeExpander(void);

  bool Lookup(CStdString& desc, const CStdString& code);
  bool Lookup(CStdString& desc, const int code);

  void LoadUserCodes(const TiXmlElement* pRootElement);
  void Clear();
protected:


  typedef std::map<std::string, std::string> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapUser;
  
  bool LookupInDb(CStdString& desc, const CStdString& code);
  bool LookupInMap(CStdString& desc, const CStdString& code);
  };

extern CLangCodeExpander g_LangCodeExpander;
