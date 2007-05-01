/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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

