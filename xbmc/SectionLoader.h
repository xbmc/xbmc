#pragma once

class CSectionLoader
{
public:
  class CSection
  {
    public:
        CStdString m_strSectionName;
        long   m_lReferenceCount;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
	static void UnloadAll();
protected:
  vector<CSection> m_vecLoadedSections;
  typedef vector<CSection>::iterator ivecLoadedSections;
};
extern class CSectionLoader g_sectionLoader;
