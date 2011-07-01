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

#ifndef __COREAUDIOAESOUND_H__
#define __COREAUDIOAESOUND_H__

#include "utils/StdString.h"
#include "AESound.h"
#include "AEWAVLoader.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"

class CWAVLoader;
class CCoreAudioAESound : public IAESound
{
public:
  CCoreAudioAESound (const CStdString &filename);
  virtual ~CCoreAudioAESound();

  virtual void DeInitialize();
  virtual bool Initialize(AEAudioFormat &outputFormat);

  virtual void Play();
  virtual void Stop();
  virtual bool IsPlaying();

  virtual void  SetVolume(float volume);
  virtual float GetVolume();
  virtual void  SetFreeCallback(AECBFunc *func, void *arg);

  unsigned int GetSampleCount();

  /* must be called before initialize to be sure we have exclusive access to our samples */
  void Lock();
  void UnLock();

  /* ReleaseSamples must be called for each time GetSamples has been called */
  virtual float* GetSamples    ();
  void           ReleaseSamples();
private:
  CCriticalSection m_MutexSound;
  CStdString       m_filename;
  CAEWAVLoader     m_wavLoader;
  float            m_volume;
  int              m_inUse;
  AECBFunc        *m_freeCallback;
  void            *m_freeCallbackArg;
  bool             m_locked;
};

#endif
