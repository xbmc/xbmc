#include "stdafx.h"
#include "MusicInfoTagLoaderYM.h"
#include "Util.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderYM::CMusicInfoTagLoaderYM(void)
{
  m_ym = 0;
}

CMusicInfoTagLoaderYM::~CMusicInfoTagLoaderYM()
{
}

bool CMusicInfoTagLoaderYM::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  m_ym = m_dll.LoadYM(strFileName.c_str());
  if (!m_ym)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderYM: failed to open YM %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.GetTitle(m_ym); // no alloc
  if (szTitle)
    if( strcmp(szTitle,"") )
    {
      tag.SetTitle(szTitle);
      tag.SetLoaded(true);
    }

  char* szArtist = (char*)m_dll.GetArtist(m_ym); // no alloc
  if (szArtist)
    if( strcmp(szArtist,"") && tag.Loaded() )
      tag.SetArtist(szArtist);

  tag.SetDuration(m_dll.GetLength(m_ym));
  m_dll.FreeYM(m_ym);
  m_ym = 0;

  return tag.Loaded();
}
