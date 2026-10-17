/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerVideoWasm.h"

CVideoPlayerVideoWasm::CVideoPlayerVideoWasm(CMediaPipelineWasm& mediaPipeline,
                                             CProcessInfo& processInfo)
  : IDVDStreamPlayerVideo(processInfo),
    m_mediaPipeline(mediaPipeline)
{
}

void CVideoPlayerVideoWasm::FlushMessages()
{
  m_mediaPipeline.FlushVideoMessages();
}

bool CVideoPlayerVideoWasm::OpenStream(const CDVDStreamInfo hint)
{
  return m_mediaPipeline.OpenVideoStream(hint);
}

void CVideoPlayerVideoWasm::CloseStream(const bool waitForBuffers)
{
  m_mediaPipeline.CloseVideoStream();
}

void CVideoPlayerVideoWasm::Flush(const bool sync)
{
  m_mediaPipeline.Flush();
}

bool CVideoPlayerVideoWasm::AcceptsData() const
{
  return m_mediaPipeline.AcceptsVideoData();
}

bool CVideoPlayerVideoWasm::HasData() const
{
  return m_mediaPipeline.HasVideoData();
}

bool CVideoPlayerVideoWasm::IsInited() const
{
  return m_mediaPipeline.IsVideoInited();
}

void CVideoPlayerVideoWasm::SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority)
{
  m_mediaPipeline.SendVideoMessage(msg, priority);
}

void CVideoPlayerVideoWasm::EnableSubtitle(const bool enable)
{
  m_mediaPipeline.EnableSubtitle(enable);
}

bool CVideoPlayerVideoWasm::IsSubtitleEnabled()
{
  return m_mediaPipeline.IsSubtitleEnabled();
}

double CVideoPlayerVideoWasm::GetSubtitleDelay()
{
  return m_mediaPipeline.GetSubtitleDelay();
}

void CVideoPlayerVideoWasm::SetSubtitleDelay(const double delay)
{
  m_mediaPipeline.SetSubtitleDelay(delay);
}

bool CVideoPlayerVideoWasm::IsStalled() const
{
  return m_mediaPipeline.IsStalled();
}

double CVideoPlayerVideoWasm::GetCurrentPts()
{
  return m_mediaPipeline.GetCurrentPts();
}

double CVideoPlayerVideoWasm::GetOutputDelay()
{
  return 0.0;
}
std::string CVideoPlayerVideoWasm::GetPlayerInfo()
{
  return m_mediaPipeline.GetVideoInfo();
}

int CVideoPlayerVideoWasm::GetVideoBitrate()
{
  return m_mediaPipeline.GetVideoBitrate();
}

void CVideoPlayerVideoWasm::SetSpeed(const int speed)
{
  m_mediaPipeline.SetSpeed(speed);
}
