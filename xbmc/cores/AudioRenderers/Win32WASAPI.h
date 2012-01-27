/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#ifndef __WIN32WASAPI_H__
#define __WIN32WASAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "IAudioRenderer.h"
#include "threads/CriticalSection.h"
#include "utils/PCMAmplifier.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CWin32WASAPI : public IAudioRenderer
{
public:
  CWin32WASAPI();
  virtual ~CWin32WASAPI();
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual unsigned int GetChunkLen();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();
  virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, EEncoded encoded = IAudioRenderer::ENCODED_NONE);

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
  void AddDataToBuffer(unsigned char* pData, unsigned int len, unsigned char* pOut);
  void UpdateCacheStatus();
  void CheckPlayStatus();
  void BuildChannelMapping(int channels, enum PCMChannels* map);

  IMMDevice* m_pDevice;
  IAudioClient* m_pAudioClient;
  IAudioRenderClient* m_pRenderClient;

  IAudioCallback* m_pCallback;

  long m_nCurrentVolume;
  long m_drc;
  float m_fVolAdjustFactor;

  unsigned int m_uiChunkSize;
  unsigned int m_uiSrcChunkSize;
  unsigned int m_uiBufferLen;
  unsigned int m_uiBytesPerFrame;
  unsigned int m_uiBytesPerSrcFrame;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  unsigned int m_uiAvgBytesPerSec;
  unsigned int m_uiSpeakerMask;
  enum PCMChannels m_SpeakerOrder[8];

  static bool m_bIsAllocated;
  bool m_bPlaying;
  bool m_bPause;
  bool m_bMuting;
  bool m_bPassthrough;

  unsigned int m_CacheLen;
  unsigned int m_LastCacheCheck;
  size_t m_PreCacheSize;

  CPCMAmplifier m_pcmAmplifier;
  CCriticalSection m_critSection;
};

#endif //__WIN32WASAPI_H__
