#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Interfaces/AESink.h"
#include <stdint.h>
#include <dsound.h>
#include "../Utils/AEDeviceInfo.h"

#include "threads/CriticalSection.h"

class CAESinkDirectSound : public IAESink
{
public:
  virtual const char *GetName() { return "DirectSound"; }

  CAESinkDirectSound();
  virtual ~CAESinkDirectSound();

  virtual bool Initialize  (AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const std::string device);

  virtual void         Stop               ();
  virtual double       GetDelay           ();
  virtual double       GetCacheTime       ();
  virtual double       GetCacheTotal      ();
  virtual unsigned int AddPackets         (uint8_t *data, unsigned int frames, bool hasAudio);
  static  void         EnumerateDevicesEx (AEDeviceInfoList &deviceInfoList);
private:
  void          AEChannelsFromSpeakerMask(DWORD speakers);
  DWORD         SpeakerMaskFromAEChannels(const CAEChannelInfo &channels);
  void          CheckPlayStatus();
  bool          UpdateCacheStatus();
  unsigned int  GetSpace();
  const char    *dserr2str(int err);

  static const char  *WASAPIErrToStr(HRESULT err);

  LPDIRECTSOUNDBUFFER m_pBuffer;
  LPDIRECTSOUND8      m_pDSound;

  AEAudioFormat       m_format;
  enum AEDataFormat   m_encodedFormat;
  CAEChannelInfo      m_channelLayout;
  std::string         m_device;

  unsigned int        m_AvgBytesPerSec;

  unsigned int        m_dwChunkSize;
  unsigned int        m_dwFrameSize;
  unsigned int        m_dwBufferLen;

  unsigned int        m_BufferOffset;
  unsigned int        m_CacheLen;
  unsigned int        m_LastCacheCheck;
  unsigned int        m_BufferTimeouts;

  bool                m_running;
  bool                m_initialized;
  bool                m_isDirtyDS;
  CCriticalSection    m_runLock;
};
