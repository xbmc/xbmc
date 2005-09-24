#include "stdafx.h"
#include "MusicInfoTagLoaderSPC.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderSPC::CMusicInfoTagLoaderSPC(void)
{
  m_spc = 0;
}

CMusicInfoTagLoaderSPC::~CMusicInfoTagLoaderSPC()
{
}

bool CMusicInfoTagLoaderSPC::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;
  
  m_dll.Init();

  m_spc = m_dll.LoadSPC(strFileName.c_str());
  if (!m_spc)
  {
    CLog::Log(LOGERROR,"MusicInfoTagLoaderSPC: failed to open SPC %s",strFileName.c_str());
    return false;
  }

  tag.SetURL(strFileName);
    
  tag.SetLoaded(false);
  char* szTitle = (char*)m_dll.GetTitle(m_spc); // no alloc
  if (szTitle)
    if( strcmp(szTitle,"") ) 
    {
      tag.SetTitle(szTitle);
      tag.SetLoaded(true);
    }

  char* szArtist = (char*)m_dll.GetArtist(m_spc); // no alloc
  if (szArtist)
    if( strcmp(szArtist,"") && tag.Loaded() )
      tag.SetArtist(szArtist);

  m_dll.FreeSPC(m_spc);
  m_spc = 0;

  return tag.Loaded();
}
