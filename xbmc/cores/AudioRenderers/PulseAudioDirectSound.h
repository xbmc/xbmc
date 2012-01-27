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

#include "IAudioRenderer.h"
#include "cores/IAudioCallback.h"

#include <pulse/pulseaudio.h>

#include "../../utils/PCMAmplifier.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();

class CPulseAudioDirectSound : public IAudioRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual unsigned int GetChunkLen();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();
  CPulseAudioDirectSound();
  virtual bool Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic=false, EEncoded encoded = IAudioRenderer::ENCODED_NONE);
  virtual ~CPulseAudioDirectSound();

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

  virtual void Flush();

  static void EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough);
private:
  static bool SetupContext(const char *host, pa_context **context, pa_threaded_mainloop **mainloop);
  bool Cork(bool cork);
  static inline bool WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry);

  IAudioCallback* m_pCallback;

  long m_nCurrentVolume;
  long m_drc;
  unsigned int m_dwPacketSize;
  unsigned int m_dwNumPackets;

  bool m_bIsAllocated;

  unsigned int m_uiBytesPerSecond;
  unsigned int m_uiBufferSize;
  unsigned int m_uiSamplesPerSec;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiDataChannels;
  unsigned int m_uiChannels;
  bool m_bPause, m_bRecentlyFlushed, m_bAutoResume;
  bool m_bPassthrough;

  pa_stream *m_Stream;
  pa_cvolume m_Volume;

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;
};

#endif

