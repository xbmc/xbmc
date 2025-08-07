/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerVideoWebOS.h"

CVideoPlayerVideoWebOS::CVideoPlayerVideoWebOS(CMediaPipelineWebOS& mediaPipeline,
                                               CProcessInfo& processInfo)
  : IDVDStreamPlayerVideo(processInfo), m_mediaPipeline(mediaPipeline)
{
}

void CVideoPlayerVideoWebOS::FlushMessages()
{
  m_mediaPipeline.FlushVideoMessages();
}

bool CVideoPlayerVideoWebOS::OpenStream(const CDVDStreamInfo hint)
{
  return m_mediaPipeline.OpenVideoStream(hint);
}

void CVideoPlayerVideoWebOS::CloseStream(const bool waitForBuffers)
{
  m_mediaPipeline.CloseVideoStream(waitForBuffers);
}

void CVideoPlayerVideoWebOS::Flush(const bool sync)
{
  m_mediaPipeline.Flush(sync);
}

bool CVideoPlayerVideoWebOS::AcceptsData() const
{
  return m_mediaPipeline.AcceptsVideoData();
}

bool CVideoPlayerVideoWebOS::HasData() const
{
  return m_mediaPipeline.HasVideoData();
}

bool CVideoPlayerVideoWebOS::IsInited() const
{
  return m_mediaPipeline.IsVideoInited();
}

void CVideoPlayerVideoWebOS::SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority)
{
  m_mediaPipeline.SendVideoMessage(msg, priority);
}

void CVideoPlayerVideoWebOS::EnableSubtitle(const bool enable)
{
  m_mediaPipeline.EnableSubtitle(enable);
}

bool CVideoPlayerVideoWebOS::IsSubtitleEnabled()
{
  return m_mediaPipeline.IsSubtitleEnabled();
}

double CVideoPlayerVideoWebOS::GetSubtitleDelay()
{
  return m_mediaPipeline.GetSubtitleDelay();
}

void CVideoPlayerVideoWebOS::SetSubtitleDelay(const double delay)
{
  m_mediaPipeline.SetSubtitleDelay(delay);
}

bool CVideoPlayerVideoWebOS::IsStalled() const
{
  return m_mediaPipeline.IsStalled();
}

double CVideoPlayerVideoWebOS::GetCurrentPts()
{
  return m_mediaPipeline.GetCurrentPts();
}

double CVideoPlayerVideoWebOS::GetOutputDelay()
{
  return 0.0;
}
std::string CVideoPlayerVideoWebOS::GetPlayerInfo()
{
  return m_mediaPipeline.GetVideoInfo();
}

int CVideoPlayerVideoWebOS::GetVideoBitrate()
{
  return m_mediaPipeline.GetVideoBitrate();
}

void CVideoPlayerVideoWebOS::SetSpeed(const int speed)
{
  m_mediaPipeline.SetSpeed(speed);
}
