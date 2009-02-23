/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// AsyncAudioRenderer.h: interface for the CAsyncDirectSound class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __PORTAUDIO_DIRECT_SOUND_H__
#define __PORTAUDIO_DIRECT_SOUND_H__

#include "portaudio.h"
#include "IDirectSoundRenderer.h"
#include "IAudioCallback.h"
#include "../ssrc.h"
#include "../../utils/PCMAmplifier.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class PortAudioDirectSound : public IDirectSoundRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  PortAudioDirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec = "", bool bIsMusic=false, bool bPassthrough = false);
  virtual ~PortAudioDirectSound();

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

  bool IsValid() { return m_pStream != 0; }
  
private:
  PaStream*  m_pStream;
  //snd_pcm_uframes_t 	m_maxFrames;

  IAudioCallback* m_pCallback;

  CPCMAmplifier 	m_amp;
  LONG m_nCurrentVolume;
  DWORD m_dwPacketSize;
  DWORD m_dwNumPackets;
  bool m_bPause;
  bool m_bIsAllocated;
  bool m_bCanPause;

  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;

  //snd_pcm_uframes_t m_BufferSize;

  bool m_bPassthrough;
};

#endif 

