/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerAudioWebOS.h"

CVideoPlayerAudioWebOS::CVideoPlayerAudioWebOS(CMediaPipelineWebOS& mediaPipeline,
                                               CProcessInfo& processInfo)
  : IDVDStreamPlayerAudio(processInfo), m_mediaPipeline(mediaPipeline)
{
}

void CVideoPlayerAudioWebOS::FlushMessages()
{
  m_mediaPipeline.FlushAudioMessages();
}

bool CVideoPlayerAudioWebOS::OpenStream(CDVDStreamInfo hints)
{
  return m_mediaPipeline.OpenAudioStream(hints);
}

void CVideoPlayerAudioWebOS::CloseStream(const bool waitForBuffers)
{
  m_mediaPipeline.CloseAudioStream(waitForBuffers);
}

void CVideoPlayerAudioWebOS::SetSpeed(const int speed)
{
  m_mediaPipeline.SetSpeed(speed);
}

void CVideoPlayerAudioWebOS::Flush(const bool sync)
{
  m_mediaPipeline.Flush(sync);
}

bool CVideoPlayerAudioWebOS::AcceptsData() const
{
  return m_mediaPipeline.AcceptsAudioData();
}

bool CVideoPlayerAudioWebOS::HasData() const
{
  return m_mediaPipeline.HasAudioData();
}

int CVideoPlayerAudioWebOS::GetLevel() const
{
  return m_mediaPipeline.GetAudioLevel();
}

bool CVideoPlayerAudioWebOS::IsInited() const
{
  return m_mediaPipeline.IsAudioInited();
}

void CVideoPlayerAudioWebOS::SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority)
{
  m_mediaPipeline.SendAudioMessage(msg, priority);
}

void CVideoPlayerAudioWebOS::SetDynamicRangeCompression(long drc)
{
}

std::string CVideoPlayerAudioWebOS::GetPlayerInfo()
{
  return m_mediaPipeline.GetAudioInfo();
}

int CVideoPlayerAudioWebOS::GetAudioChannels()
{
  return m_mediaPipeline.GetAudioChannels();
}

double CVideoPlayerAudioWebOS::GetCurrentPts()
{
  return m_mediaPipeline.GetCurrentPts();
}

bool CVideoPlayerAudioWebOS::IsStalled() const
{
  return m_mediaPipeline.IsStalled();
}

bool CVideoPlayerAudioWebOS::IsPassthrough() const
{
  return true;
}

float CVideoPlayerAudioWebOS::GetDynamicRangeAmplification() const
{
  return 0.0f;
}
