/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AESound.h"

class DllAvUtil;

namespace XFILE
{
class CFile;
}

namespace ActiveAE
{

class CActiveAE;

class CActiveAESound : public IAESound
{
public:
  CActiveAESound (const std::string &filename, CActiveAE *ae);
  ~CActiveAESound() override;

  void Play() override;
  void Stop() override;
  bool IsPlaying() override;

  void SetChannel(AEChannel channel) override { m_channel = channel; }
  AEChannel GetChannel() override { return m_channel; }
  void SetVolume(float volume) override { m_volume = std::max(0.0f, std::min(1.0f, volume)); }
  float GetVolume() override { return m_volume; }

  uint8_t** InitSound(bool orig, SampleConfig config, int nb_samples);
  bool StoreSound(bool orig, uint8_t **buffer, int samples, int linesize);
  CSoundPacket *GetSound(bool orig);

  bool IsConverted() { return m_isConverted; }
  void SetConverted(bool state) { m_isConverted = state; }

  bool Prepare();
  void Finish();
  int GetChunkSize();
  int GetFileSize() { return m_fileSize; }
  bool IsSeekPossible() { return m_isSeekPossible; }

  static int Read(void *h, uint8_t* buf, int size);
  static int64_t Seek(void *h, int64_t pos, int whence);

protected:
  CActiveAE *m_activeAE;
  std::string m_filename;
  XFILE::CFile *m_pFile;
  bool m_isSeekPossible;
  int m_fileSize;
  float m_volume = 1.0f;
  AEChannel m_channel = AE_CH_NULL;

  CSoundPacket *m_orig_sound;
  CSoundPacket *m_dst_sound;

  bool m_isConverted;
};
}
