// IAudioCallback.h: interface for the IAudioCallback class.
//
//////////////////////////////////////////////////////////////////////

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

class IAudioCallback
{
public:
  IAudioCallback() = default;
  virtual ~IAudioCallback() = default;
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample) = 0;
  virtual void OnAudioData(const float* pAudioData, unsigned int iAudioDataLength) = 0;
};

