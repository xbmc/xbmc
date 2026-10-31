/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerAudioWasm.h"

CVideoPlayerAudioWasm::CVideoPlayerAudioWasm(CMediaPipelineWasm& mediaPipeline,
                                             CProcessInfo& processInfo)
  : IDVDStreamPlayerAudio(processInfo),
    m_mediaPipeline(mediaPipeline)
{
}

void CVideoPlayerAudioWasm::FlushMessages()
{
  m_mediaPipeline.FlushAudioMessages();
}

bool CVideoPlayerAudioWasm::OpenStream(CDVDStreamInfo hints)
{
  return m_mediaPipeline.OpenAudioStream(hints);
}

void CVideoPlayerAudioWasm::CloseStream(const bool waitForBuffers)
{
  m_mediaPipeline.CloseAudioStream();
}

void CVideoPlayerAudioWasm::SetSpeed(const int speed)
{
  m_mediaPipeline.SetSpeed(speed);
}

void CVideoPlayerAudioWasm::Flush(const bool sync)
{
  // VideoPlayer calls this right before calling flush on the video player
  // We can skip this call because it would essentially flush the pipeline twice
}

bool CVideoPlayerAudioWasm::AcceptsData() const
{
  return m_mediaPipeline.AcceptsAudioData();
}

bool CVideoPlayerAudioWasm::HasData() const
{
  return m_mediaPipeline.HasAudioData();
}

int CVideoPlayerAudioWasm::GetLevel() const
{
  return m_mediaPipeline.GetAudioLevel();
}

bool CVideoPlayerAudioWasm::IsInited() const
{
  return m_mediaPipeline.IsAudioInited();
}

void CVideoPlayerAudioWasm::SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority)
{
  m_mediaPipeline.SendAudioMessage(msg, priority);
}

void CVideoPlayerAudioWasm::SetDynamicRangeCompression(const long drc)
{
  // The backend decodes the audio.
}

std::string CVideoPlayerAudioWasm::GetPlayerInfo()
{
  return m_mediaPipeline.GetAudioInfo();
}

int CVideoPlayerAudioWasm::GetAudioChannels()
{
  return m_mediaPipeline.GetAudioChannels();
}

double CVideoPlayerAudioWasm::GetCurrentPts()
{
  return m_mediaPipeline.GetCurrentPts();
}

bool CVideoPlayerAudioWasm::IsStalled() const
{
  return m_mediaPipeline.IsStalled();
}

bool CVideoPlayerAudioWasm::IsPassthrough() const
{
  // Kodi's volume does not apply to a backend without volume.
  return !m_mediaPipeline.Backend().HasVolume();
}

float CVideoPlayerAudioWasm::GetDynamicRangeAmplification() const
{
  return 0.0f;
}
