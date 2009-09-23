/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "MusicInfoTagLoaderFlac.h"
#include "FlacTag.h"
#include "utils/log.h"


using namespace MUSIC_INFO;

CMusicInfoTagLoaderFlac::CMusicInfoTagLoaderFlac(void)
{}

CMusicInfoTagLoaderFlac::~CMusicInfoTagLoaderFlac()
{}

bool CMusicInfoTagLoaderFlac::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the Flac Tag info from strFileName
    // and put it in tag
    CFlacTag myTag;
    if (myTag.Read(strFileName))
    {
      myTag.GetMusicInfoTag(tag);
    }
    return tag.Loaded();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader flac: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
