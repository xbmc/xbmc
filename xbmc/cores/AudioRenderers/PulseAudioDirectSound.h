/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __PULSE_AUDIO_DIRECT_SOUND_H__
#define __PULSE_AUDIO_DIRECT_SOUND_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IDirectSoundRenderer.h"
#include "IAudioCallback.h"
#include "../ssrc.h"

#include <pulse/pulseaudio.h>

#include "../../utils/PCMAmplifier.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CPulseAudioDirectSound : public IDirectSoundRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  CPulseAudioDirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec = "", bool bIsMusic=false, bool bPassthrough = false);
  virtual ~CPulseAudioDirectSound();

  virtual DWORD AddPackets(unsigned char* data, DWORD len);
  virtual DWORD GetSpace();
  virtual HRESULT Deinitialize();
  virtual HRESULT Pause();
  virtual HRESULT Stop();
  virtual HRESULT Resume();

  virtual LONG GetMinimumVolume() const;
  virtual LONG GetMaximumVolume() const;
  virtual LONG GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual HRESULT SetCurrentVolume(LONG nVolume);
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  virtual void Flush();
private:
  IAudioCallback* m_pCallback;

  LONG m_nCurrentVolume;
  DWORD m_dwPacketSize;
  DWORD m_dwNumPackets;
  
  bool m_bIsAllocated;
  bool m_bCanPause;

  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  bool m_bPause;
  bool m_bPassthrough;

  pa_threaded_mainloop *m_MainLoop;
  pa_stream *m_Stream;
  pa_context *m_Context;
  pa_sample_spec m_SampleSpec;
  pa_cvolume m_Volume;
};

#endif 

