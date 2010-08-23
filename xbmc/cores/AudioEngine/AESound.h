#pragma once
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

#include "StdString.h"
#include "utils/CriticalSection.h"

class CAESound
{
public:
  void DeInitialize();
  bool Initialize();

  void   Play();
  void   Stop();
  bool   IsPlaying();

  void         SetVolume     (float volume) { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  float        GetVolume     ()             { return m_volume      ; }
  unsigned int GetSampleCount()             { return m_sampleCount ; }
  float*       GetSamples    ();
private:
  /* only the AE can create these objects */
  friend class CAE;
  CAESound(const CStdString &filename);
  ~CAESound();

  CCriticalSection m_critSection;
  CStdString       m_filename;
  int              m_refcount; /* used for GC */
  unsigned int     m_ts;       /* used for GC */
  bool             m_valid;
  unsigned int     m_channelCount;
  float           *m_samples;
  unsigned int     m_frameCount;
  unsigned int     m_sampleCount;
  float            m_volume;
};

