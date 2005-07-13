#pragma once

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
  void Clear();
protected:


struct cstring_less_than : std::binary_function<const char*, const char*, bool>
  {
    bool operator()(const char* lhs, const char* rhs) const
    {
      return (strcmp(lhs, rhs) < 0);
    }
  };

  typedef std::map<const char *, const char *, cstring_less_than > STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapISO639_1;
  STRINGLOOKUPTABLE m_mapISO639_2;
  STRINGLOOKUPTABLE m_mapUser;

  inline bool LookupInMap(CStdString& desc, const CStdString& code, STRINGLOOKUPTABLE& slmap);
  inline void ClearMap(STRINGLOOKUPTABLE& slmap);

  void LoadCodes(const TiXmlElement* pRootElement, STRINGLOOKUPTABLE& m_map);
  void LoadCodes(const CStdString& strXMLFile, STRINGLOOKUPTABLE& m_map);
};

extern CLangCodeExpander g_LangCodeExpander;
