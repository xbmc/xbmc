#pragma once
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

#include "cores/paplayer/ReplayGain.h"
#include "MusicInfoTag.h"

namespace MUSIC_INFO
{
class CTag
{
public:
  CTag(void) { m_art = NULL; }
  virtual ~CTag(void) {}
  virtual bool Read(const CStdString& strFile) { m_musicInfoTag.SetURL(strFile); return false; }
  virtual bool Write(const CStdString& strFile) { return false; }

  const CReplayGain &GetReplayGain() const { return m_replayGain; }
  void GetMusicInfoTag(CMusicInfoTag& tag) const { tag=m_musicInfoTag; }
  void SetMusicInfoTag(CMusicInfoTag& tag) { m_musicInfoTag=tag; }
  void SetArt(EmbeddedArt *art) { m_art = art; }

protected:
  CMusicInfoTag m_musicInfoTag;
  EmbeddedArt*  m_art;
  CReplayGain m_replayGain;
};
}
