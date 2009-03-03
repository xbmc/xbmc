/*
 *      Copyright (C) 2009 Team XBMC
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

#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include <vector>

#define AM_STREAM_FORMAT_PCM      0x0001
#define AM_STREAM_FORMAT_FLOAT    0x0002
#define AM_STREAM_FORMAT_ENCODED  0x0003

typedef unsigned int AM_STREAM_ID;

#define AM_MIXER_HARDWARE 1
#define AM_MIXER_SOFTWARE 2

class CAudioStream
{
public:
  CAudioStream();
  virtual ~CAudioStream();
  bool Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, int mixerChannel, IAudioSink* pMixerSink);
  CStreamInput* GetInput();
  CDSPChain* GetDSPChain();
  int GetMixerChannel();
  bool NeedsData();
  unsigned int GetCacheSize();
  unsigned int GetCacheTime();
  bool ProcessStream();
private:
  CStreamInput* m_pInput;
  CDSPChain* m_pDSPChain;
  int m_MixerChannel;
  IAudioSink* m_pMixerSink;
  audio_slice* m_pInputSourceSlice;
  audio_slice* m_pDSPSourceSlice;

};

typedef std::vector<CAudioStream*>::iterator StreamIterator;

class CAudioManager
{
public:
  CAudioManager();
  virtual ~CAudioManager();
  AM_STREAM_ID OpenStream(CStreamDescriptor* pDesc, size_t blockSize);  // Writes must occur in whole-block chunks
  void CloseStream(AM_STREAM_ID streamId);
  int AddDataToStream(AM_STREAM_ID streamId, void* pData, size_t len);
  bool ControlStream(AM_STREAM_ID streamId, int controlCode);
  bool SetStreamVolume(AM_STREAM_ID streamId, long vol);
  bool SetStreamProp(AM_STREAM_ID streamId, int propId, const void* pVal);
  bool GetStreamProp(AM_STREAM_ID streamId, int propId, void* pVal);
  float GetStreamDelay(AM_STREAM_ID streamId);
  void DrainStream(AM_STREAM_ID streamId, unsigned int maxTime); // Play all slices that have been received but not rendered yet (finish by maxTime)
  void FlushStream(AM_STREAM_ID streamId);  // Drop any slices that have been received but not rendered yet
  bool SetMixerType(int mixerType);

protected:
  std::vector<CAudioStream*> m_StreamList;
  IAudioMixer* m_pMixer;

  bool AddInputStream(CAudioStream* pStream);
  CAudioStream* GetInputStream(AM_STREAM_ID streamId);
  AM_STREAM_ID GetStreamId(CAudioStream* pStream);
  int GetOpenStreamCount();
};

extern CAudioManager g_AudioLibManager;

#endif // __AUDIO_MANAGER_H__