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
#include "AESound.h"
#include "AEWAVLoader.h"

class CWAVLoader;
class CSoftAESound : public IAESound
{
public:
  CSoftAESound (const CStdString &filename);
  virtual ~CSoftAESound();

  virtual void DeInitialize();
  virtual bool Initialize();

  virtual void   Play();
  virtual void   Stop();
  virtual bool   IsPlaying();

  virtual void         SetVolume     (float volume) { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  virtual float        GetVolume     ()             { return m_volume      ; }
  virtual unsigned int GetSampleCount();
  virtual float*       GetSamples    ();

  int          GetRefCount() { return m_refcount; }
  void         IncRefCount() { ++m_refcount; }
  void         DecRefCount() { --m_refcount; ASSERT(m_refcount >= 0); }
  void         SetTimeout(unsigned int ts) { m_ts = ts; }
  unsigned int GetTimeout() { return m_ts; }

private:
  CCriticalSection m_critSection;
  CStdString       m_filename;
  CAEWAVLoader     m_wavLoader;
  int              m_refcount; /* used for GC */
  unsigned int     m_ts;       /* used for GC */
  float            m_volume;
};

