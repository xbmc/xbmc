/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://xbmc.org
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

#ifndef AESOUND_H
#define AESOUND_H

#include "StdString.h"
#include "utils/CriticalSection.h"

using namespace std;

class CAESound
{
public:
  CAESound(const CStdString &filename);
  ~CAESound();

  void DeInitialize();
  bool Initialize();

  void   Play();
  void   Stop();
  bool   IsPlaying();

  void   SetVolume(float volume) { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  float  GetVolume()             { return m_volume ; }
  float* GetFrame(unsigned int frame);
private:
  CCriticalSection m_critSection;
  CStdString       m_filename;
  bool             m_valid;
  unsigned int     m_channelCount;
  float           *m_samples;
  unsigned int     m_frameCount;
  float            m_volume;
};

#endif

