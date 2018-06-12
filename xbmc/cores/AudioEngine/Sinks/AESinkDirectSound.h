/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <stdint.h>
#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "threads/CriticalSection.h"

#include <mmsystem.h>
#include <DSound.h>
#include <wrl/client.h>

class CAESinkDirectSound : public IAESink
{
public:
  virtual const char *GetName() { return "DIRECTSOUND"; }

  CAESinkDirectSound();
  virtual ~CAESinkDirectSound();

  static void Register();
  static IAESink* Create(std::string &device, AEAudioFormat &desiredFormat);

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();

  virtual void Stop();
  virtual void Drain();
  virtual void GetDelay(AEDelayStatus& status);
  virtual double GetCacheTotal();
  virtual unsigned int AddPackets(uint8_t **data, unsigned int frames, unsigned int offset);

  static std::string GetDefaultDevice();
  static void EnumerateDevicesEx (AEDeviceInfoList &deviceInfoList, bool force = false);
private:
  void          AEChannelsFromSpeakerMask(DWORD speakers);
  DWORD         SpeakerMaskFromAEChannels(const CAEChannelInfo &channels);
  void          CheckPlayStatus();
  bool          UpdateCacheStatus();
  unsigned int  GetSpace();
  const char    *dserr2str(int err);

  Microsoft::WRL::ComPtr<IDirectSoundBuffer> m_pBuffer;
  Microsoft::WRL::ComPtr<IDirectSound> m_pDSound;

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
