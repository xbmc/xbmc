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

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CWin32DirectSound : public IAudioRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual DWORD GetChunkLen();
  virtual FLOAT GetDelay();
  CWin32DirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec = "", bool bIsMusic=false, bool bAudioPassthrough=false);
  virtual ~CWin32DirectSound();

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

private:
  void UpdateCacheStatus();
  void MapDataIntoBuffer(unsigned char* pData, DWORD len, unsigned char* pOut);
  unsigned char* GetChannelMap(unsigned int channels, const char* strAudioCodec);

  LPDIRECTSOUNDBUFFER  m_pBuffer;
  LPDIRECTSOUND8 m_pDSound;

  IAudioCallback* m_pCallback;

  LONG m_nCurrentVolume;
  DWORD m_dwChunkSize;
  DWORD m_dwBufferLen;
  bool m_bPause;
  bool m_bIsAllocated;

  bool m_Passthrough;
  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  unsigned int m_AvgBytesPerSec;

  char * dserr2str(int err);

  unsigned int m_BufferOffset;
  unsigned int m_CacheLen;

  unsigned int m_LastCacheCheck;
  size_t m_PreCacheSize;

  unsigned char* m_pChannelMap;
};

#endif // !defined(AFX_ASYNCAUDIORENDERER_H__B590A94D_D15E_43A6_A41D_527BD441B5F5__INCLUDED_)
