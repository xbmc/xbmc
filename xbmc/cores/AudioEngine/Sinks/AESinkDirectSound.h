/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "threads/CriticalSection.h"

#include <stdint.h>

#include <mmsystem.h> /* Microsoft can't write standalone headers */
#include <DSound.h> /* Microsoft can't write standalone headers */
#include <wrl/client.h>

class CAESinkDirectSound : public IAESink
{
public:
  virtual const char *GetName() { return "DIRECTSOUND"; }

  CAESinkDirectSound();
  virtual ~CAESinkDirectSound();

  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);

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
  unsigned int        m_BufferTimeouts;

  bool                m_running;
  bool                m_initialized;
  bool                m_isDirtyDS;
  CCriticalSection    m_runLock;
};
