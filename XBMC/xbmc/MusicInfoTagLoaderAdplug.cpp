#include "stdafx.h"
#include "MusicInfoTagLoaderAdplug.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderAdplug::CMusicInfoTagLoaderAdplug(void)
{
  m_adl = 0;
}

CMusicInfoTagLoaderAdplug::~CMusicInfoTagLoaderAdplug()
{
}

bool CMusicInfoTagLoaderAdplug::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  m_adl = m_dll.LoadADL(strFileName.c_str());
  if (!m_adl)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderAdplug: failed to open %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  const char* szTitle = m_dll.GetTitle(m_adl); // no alloc
  CLog::Log(LOGDEBUG,"got title %p %s!",szTitle,szTitle);
  if (szTitle)
    if( strcmp(szTitle,"") ) 
    {
      tag.SetTitle(szTitle);
      tag.SetLoaded(true);
    }
  
  const char* szArtist = m_dll.GetArtist(m_adl); // no alloc
  CLog::Log(LOGDEBUG,"got artist %p %s!",szArtist,szArtist);
  if( strcmp(szArtist,"") && tag.Loaded() ) 
    tag.SetArtist(szArtist);
  
  tag.SetDuration(m_dll.GetLength(m_adl)/1000);
  
  CLog::Log(LOGDEBUG,"call free!");
  m_dll.FreeADL(m_adl);
  CLog::Log(LOGDEBUG,"free called!");
  m_adl = 0;

  return tag.Loaded();
}