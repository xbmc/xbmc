/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MusicInfoTagLoaderNSF.h"
#include "MusicInfoTag.h"
#include "utils/log.h"

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

bool CMusicInfoTagLoaderNSF::Load(const CStdString& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
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
  if( szTitle && strcmp(szTitle,"<?>") )
  {
    tag.SetTitle(szTitle);
    tag.SetLoaded(true);
  }

  char* szArtist = (char*)m_dll.GetArtist(m_nsf); // no alloc
  if( szArtist && strcmp(szArtist,"<?>") && tag.Loaded() )
    tag.SetArtist(szArtist);

  m_dll.FreeNSF(m_nsf);
  m_nsf = 0;

  return tag.Loaded();
}
