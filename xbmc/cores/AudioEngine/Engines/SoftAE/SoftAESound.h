#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"
#include "Interfaces/AESound.h"
#include "Utils/AEWAVLoader.h"

class CSoftAESound : public IAESound
{
public:
  CSoftAESound (const std::string &filename);
  virtual ~CSoftAESound();

  virtual void DeInitialize();
  virtual bool Initialize();

  virtual void   Play();
  virtual void   Stop();
  virtual bool   IsPlaying();

  virtual void   SetVolume(float volume) { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  virtual float  GetVolume()             { return m_volume      ; }

  bool         IsCompatible();
  unsigned int GetSampleCount();

  /* ReleaseSamples must be called for each time GetSamples has been called */
  virtual float* GetSamples    ();
  void           ReleaseSamples();
private:
  CCriticalSection m_critSection;
  std::string      m_filename;
  CAEWAVLoader     m_wavLoader;
  float            m_volume;
  int              m_inUse;
};

