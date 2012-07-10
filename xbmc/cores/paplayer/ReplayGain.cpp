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

#include "ReplayGain.h"

CReplayGain::CReplayGain()
{
  iTrackGain = 0;
  iAlbumGain = 0;
  fTrackPeak = 0.0f;
  fAlbumPeak = 0.0f;
  iHasGainInfo = 0;
}

CReplayGain::~CReplayGain()
{
}

void CReplayGain::SetTrackGain(int trackGain)
{
  iTrackGain = trackGain;
  iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
}

void CReplayGain::SetAlbumGain(int albumGain)
{
  iTrackGain = albumGain;
  iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
}

void CReplayGain::SetTrackPeak(float trackPeak)
{
  fTrackPeak = trackPeak;
  iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
}

void CReplayGain::SetAlbumPeak(float albumPeak)
{
  fAlbumPeak = albumPeak;
  iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
}
