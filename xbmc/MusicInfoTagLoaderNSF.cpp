#include "stdafx.h"
#include "MusicInfoTagLoaderNSF.h"
#include "lib/mikxbox/mikmod.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderNSF::CMusicInfoTagLoaderNSF(void)
{
  m_nsf = 0;
}

CMusicInfoTagLoaderNSF::~CMusicInfoTagLoaderNSF()
{
}

int CMusicInfoTagLoaderNSF::GetStreamCount(const CStdString& strFileName)
{
  if (!m_dll.Load())
    return 0;
   
  m_nsf = m_dll.LoadNSF(strFileName.c_str());
  if (!m_nsf)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderNSF: failed to open NSF %s",strFileName.c_str());
    return 0;
  }
  int result = m_dll.GetNumberOfSongs(m_nsf);
  m_dll.FreeNSF(m_nsf);
  
  return result;
}

bool CMusicInfoTagLoaderNSF::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  m_nsf = m_dll.LoadNSF(strFileName.c_str());
  if (!m_nsf)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderNSF: failed to open NSF %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.GetTitle(m_nsf); // no alloc
  if( strcmp(szTitle,"<?>") ) 
  {
    tag.SetTitle(szTitle);
    tag.SetLoaded(true);
  }
  
  char* szArtist = (char*)m_dll.GetArtist(m_nsf); // no alloc
  if( strcmp(szArtist,"<?>") && tag.Loaded() ) 
    tag.SetArtist(szArtist);
  
  m_dll.FreeNSF(m_nsf);
  m_nsf = 0;

  return tag.Loaded();
}
