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

#include "MusicInfoTagLoaderWav.h"
#include "cores/paplayer/WAVcodec.h"
#include "MusicInfoTag.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderWAV::CMusicInfoTagLoaderWAV(void)
{}

CMusicInfoTagLoaderWAV::~CMusicInfoTagLoaderWAV()
{}

bool CMusicInfoTagLoaderWAV::Load(const CStdString& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  try
  {
    // WAV has no tag information other than the duration.

    // Load our codec class
    WAVCodec codec;
    if (codec.Init(strFileName, 4096))
    {
      tag.SetURL(strFileName);
      tag.SetDuration((int)(codec.m_TotalTime/1000));
      tag.SetLoaded(false);
      codec.DeInit();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader wav: exception in file %s", strFileName.c_str());
  }

  return false;
}

