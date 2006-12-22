#include "stdafx.h"
#include "MusicInfoTagLoaderGYM.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderGYM::CMusicInfoTagLoaderGYM(void)
{
  m_gym = 0;
}

CMusicInfoTagLoaderGYM::~CMusicInfoTagLoaderGYM()
{
}

bool CMusicInfoTagLoaderGYM::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;
  
  m_dll.Init();

  m_gym = m_dll.LoadGYM(strFileName.c_str());
  if (!m_gym)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderGYM: failed to open GYM %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.GetTitle(m_gym); // no alloc
  if (szTitle)
    if( strcmp(szTitle,"") )
    {
      tag.SetTitle(szTitle);
      tag.SetLoaded(true);
    }

  char* szArtist = (char*)m_dll.GetArtist(m_gym); // no alloc
  if (szArtist)
    if( strcmp(szArtist,"") && tag.Loaded() )
      tag.SetArtist(szArtist);

  m_dll.FreeGYM(m_gym);
  m_gym = 0;

  return tag.Loaded();
}
