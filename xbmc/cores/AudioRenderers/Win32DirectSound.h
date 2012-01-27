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

#if !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
#define AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAudioRenderer.h"
#include "threads/CriticalSection.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CWin32DirectSound : public IAudioRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual unsigned int GetChunkLen();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();
  CWin32DirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels* channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, EEncoded encoded = IAudioRenderer::ENCODED_NONE);
  virtual ~CWin32DirectSound();

  virtual unsigned int AddPackets(const void* data, unsigned int len);
  virtual unsigned int GetSpace();
  virtual bool Deinitialize();
  virtual bool Pause();
  virtual bool Stop();
  virtual bool Resume();

  virtual long GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual bool SetCurrentVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc) { m_drc = drc; }
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  static void EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough);

private:
  void UpdateCacheStatus();
  void CheckPlayStatus();
  void BuildChannelMapping(int channels, enum PCMChannels* map);

  LPDIRECTSOUNDBUFFER  m_pBuffer;
  LPDIRECTSOUND8 m_pDSound;

  IAudioCallback* m_pCallback;

  long m_nCurrentVolume;
  long m_drc;
  unsigned int m_dwChunkSize;
  unsigned int m_dwDataChunkSize;
  unsigned int m_dwBufferLen;
  bool m_bPause;
  bool m_bIsAllocated;

  bool m_Passthrough;
  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  unsigned int m_uiDataChannels;
  unsigned int m_AvgBytesPerSec;
  unsigned int m_uiBytesPerFrame;
  unsigned int m_uiDataBytesPerFrame;
  unsigned int m_uiSpeakerMask;
  enum PCMChannels m_SpeakerOrder[8];

  char * dserr2str(int err);

  unsigned int m_BufferOffset;
  unsigned int m_CacheLen;

  unsigned int m_LastCacheCheck;
  size_t m_PreCacheSize;

  CCriticalSection m_critSection;
};

#endif // !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
