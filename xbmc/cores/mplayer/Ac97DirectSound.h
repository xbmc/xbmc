/*
* XBoxMediaPlayer
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

// AsyncAudioRenderer.h: interface for the CAc97DirectSound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AC97AUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
#define AFX_AC97AUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IDirectSoundRenderer.h"
#include "IAudioCallback.h"
#include "cores/ssrc.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CAc97DirectSound : public IDirectSoundRenderer
{
public:
  CAc97DirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bAC3DTS = true);
  virtual ~CAc97DirectSound();

  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
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
  static void CALLBACK StaticStreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus);
  void StreamCallback(LPVOID pPacketContext, DWORD dwStatus);
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers){};

private:
  IAudioCallback* m_pCallback;
  LONG m_lFadeVolume;
  bool FindFreePacket( DWORD& pdwIndex );

  LPAC97MEDIAOBJECT m_pDigitalOutput;  
  DWORD m_dwPacketSize;
  DWORD m_dwNumPackets;
  PBYTE m_pbSampleData[64];
  DWORD* m_adwStatus;
  DWORD m_dwTotalBytesAdded;
  bool m_bPause;
  bool m_bMute;
  bool m_bIsAllocated;  
  LPDIRECTSOUND8 m_pDSound;

  //add for 44.1KHz 2 Channel audio Passthrough after software resample
  bool m_bAc3DTS; //input stream property
};

#endif // !defined(AFX_AC97AUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
