/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
// AudioDSPSettings.h: interface for the CAudioSettings class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "addons/DllAudioDSP.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

class CAudioSettings
{
public:
  CAudioSettings();
  ~CAudioSettings() {};

  bool operator!=(const CAudioSettings &right) const;

  int m_MasterStreamTypeSel;
  int m_MasterStreamType;
  int m_MasterStreamBase;
  int m_MasterModes[AE_DSP_ASTREAM_MAX][AE_DSP_ABASE_MAX];

private:
};
