#include "sectionloader.h"
#include "stdstring.h"
#include <xtl.h>


class CSectionLoader g_sectionLoader;

CSectionLoader::CSectionLoader(void)
{
}

CSectionLoader::~CSectionLoader(void)
{
}

bool CSectionLoader::IsLoaded(const CStdString& strSection)
{
  for (int i=0; i < (int)g_sectionLoader.m_vecLoadedSections.size(); ++i)
  {
    CSection& section=g_sectionLoader.m_vecLoadedSections[i];
    if (section.m_strSectionName==strSection) return true;
  }
  return false;
}

bool CSectionLoader::Load(const CStdString& strSection)
{
  if (CSectionLoader::IsLoaded(strSection)) return true;

	CStdString strLog;
  if ( NULL==XLoadSection(strSection.c_str() ) )
  {
		strLog.Format("Section %s load failed!!\n", strSection.c_str());
    OutputDebugString(strLog.c_str() );
    return false;
  }
	HANDLE hHandle=XGetSectionHandle(strSection.c_str());

	strLog.Format("Section %s loaded size:%i\n", strSection.c_str(),XGetSectionSize(hHandle) );
  OutputDebugString(strLog.c_str() );
	
  CSection newSection;
  newSection.m_strSectionName=strSection;
  newSection.m_lReferenceCount=1;
  g_sectionLoader.m_vecLoadedSections.push_back(newSection);
  return true;
}

void CSectionLoader::Unload(const CStdString& strSection)
{
  if (!CSectionLoader::IsLoaded(strSection)) return ;

  


  ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section=*i;
    if (section.m_strSectionName==strSection)
    {
      section.m_lReferenceCount--;
      if ( 0 == section.m_lReferenceCount)
      {
        g_sectionLoader.m_vecLoadedSections.erase(i);
        OutputDebugString("FreeSection:");
        OutputDebugString(strSection.c_str());
        OutputDebugString("\n");
        XFreeSection(strSection.c_str());

        return;
      }
    }
    ++i;
  }
}


void CSectionLoader::UnloadAll()
{
	ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section=*i;
    
    g_sectionLoader.m_vecLoadedSections.erase(i);
    OutputDebugString("FreeSection:");
		OutputDebugString(section.m_strSectionName.c_str());
    OutputDebugString("\n");
    XFreeSection(section.m_strSectionName.c_str());
    i=g_sectionLoader.m_vecLoadedSections.erase(i);
  }
}