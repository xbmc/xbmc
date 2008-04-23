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

// AsyncAudioRenderer.h: interface for the CAsyncDirectSound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
#define AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IDirectSoundRenderer.h"
#include "IAudioCallback.h"
#include "cores/ssrc.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CASyncDirectSound : public IDirectSoundRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  CASyncDirectSound(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, const char* strAudioCodec = "", bool bIsMusic = false);
  virtual ~CASyncDirectSound();

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
  virtual void DoWork();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);
  virtual void SetDynamicRangeCompression(long drc);

private:
  void ApplyDynamicRangeCompression(void *dest, const void *source, const int bytes);

  IAudioCallback* m_pCallback;
  LONG m_lFadeVolume;

  bool FindFreePacket( DWORD& pdwIndex );

  LPDIRECTSOUNDSTREAM m_pStream;
  LPDIRECTSOUND8 m_pDSound;

  WAVEFORMATEX m_wfx;

  LONG m_nCurrentVolume;
  DWORD m_dwPacketSize;
  bool m_AudioEOF;
  DWORD m_dwNumPackets;
  PBYTE* m_pbSampleData;
  DWORD* m_adwStatus;
  LARGE_INTEGER m_LastPacketCompletedAt;
  bool m_bPause;
  bool m_bIsPlaying;
  bool m_bIsAllocated;
  bool m_bFirstPackets;
  PBYTE m_VisBuffer;
  DWORD m_VisBytes;
  DWORD m_VisMaxBytes;

  LONGLONG m_startTime;
  LONGLONG m_delay;
  FLOAT m_fCurDelay;
  int m_iCalcDelay;
  WAVEFORMATEXTENSIBLE m_wfxex;
  LONGLONG m_TicksPerSec;
  int m_iAudioSkip;
  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;

  int m_iCurrentAudioStream;   //The Variable tracking LEFT/RIGHT/STEREO

  // Dynamic range compensation stuff
  short *m_drcTable;      // lookup table for fast pow() function
  int m_drcAmount;        // amount of dynamic range compression in milliBels.
};

#endif // !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
