/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LiveGeometryMonitor.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDMessage.h"
#include "cores/VideoPlayer/DVDMessageQueue.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoFileGeometry.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "video/geometry/ContentBarDetector.h"
#include "video/geometry/GeometrySettings.h"
#include "video/geometry/GeometryTransforms.h"

#include <algorithm>

extern "C"
{
#include <libavutil/pixdesc.h>
}

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

//! \brief How often the settings store is consulted.
constexpr int64_t SETTINGS_REFRESH_MS = 1000;

//! \brief Width of the reduced copy a hardware-decoded picture is read through.
constexpr unsigned int REDUCTION_WIDTH = 960;

//! \brief Tolerates formats FFmpeg cannot name.
const char* PixelFormatName(AVPixelFormat format)
{
  const char* name = av_get_pix_fmt_name(format);
  return name ? name : "unknown";
}

} // unnamed namespace

CLiveGeometryMonitor::CLiveGeometryMonitor(CDVDMessageQueue& messageParent,
                                           CProcessInfo& processInfo)
  : m_messageParent(messageParent),
    m_processInfo(processInfo)
{
}

void CLiveGeometryMonitor::Withdraw()
{
  if (m_selector.HasPublished())
    Post({.clear = true});

  m_selector.Configure({}, 1.0f);
  m_codedWidth = 0;
  m_codedHeight = 0;
}

void CLiveGeometryMonitor::OnStreamOpened()
{
  Withdraw();

  m_settingsReadMs = 0;
  m_stereoMode.clear();
  m_unreadableLogged = false;
  m_reducedLogged = false;
  SetState("waiting for a frame");
}

void CLiveGeometryMonitor::OnFlush()
{
  m_selector.ResetForSeek();
}

CRectInt CLiveGeometryMonitor::OnPicture(const VideoPicture& picture,
                                         const CDVDStreamInfo& hints,
                                         int speed)
{
  // Every frame is measured; only the rules are re-read on a cadence.
  const int64_t now = CTimeUtils::MonotonicMs();
  if (m_settingsReadMs == 0 || now - m_settingsReadMs >= SETTINGS_REFRESH_MS)
  {
    m_settingsReadMs = now;
    m_settings = LiveGeometryFromSettings();
    m_declaredAspect = m_processInfo.GetVideoSettings().m_declaredAspect;
    m_selector.SetParams(m_settings.selector);
  }

  if (!m_settings.enabled)
  {
    Withdraw();
    SetState("off");
    return InForce();
  }

  if (speed != DVD_PLAYSPEED_NORMAL)
  {
    SetState("not reading during trick play");
    return InForce();
  }

  if (m_declaredAspect > 0.0f)
  {
    SetState("pinned by a declared ratio");
    return InForce();
  }

  if (picture.iWidth == 0 || picture.iHeight == 0)
    return InForce();

  if (picture.iWidth != m_codedWidth || picture.iHeight != m_codedHeight ||
      picture.stereoMode != m_stereoMode)
  {
    Withdraw();

    const float displayAspect =
        picture.iDisplayWidth > 0 && picture.iDisplayHeight > 0
            ? static_cast<float>(picture.iDisplayWidth) / static_cast<float>(picture.iDisplayHeight)
            : 0.0f;
    const StreamGeometry stream =
        MeasuredStreamGeometry(picture.stereoMode, picture.iWidth, picture.iHeight, displayAspect);

    m_selector.Configure(stream.coded, PixelAspectRatio(stream),
                         ContentGeometryAtRestFromSettings());
    m_codedWidth = picture.iWidth;
    m_codedHeight = picture.iHeight;
    m_stereoMode = picture.stereoMode;
  }

  FrameRef frame;
  bool reduced = false;
  if (!AcquireFrame(picture, hints, frame, reduced))
    return InForce();

  DetectionResult sample = DetectContentRect(frame);
  if (reduced)
  {
    // The reading is in the reduction's coordinates; everything served lives in coded space.
    sample.rect = ScaleRect(sample.rect, m_reduction.width, m_reduction.height, picture.iWidth,
                            picture.iHeight);
  }

  const auto served = m_selector.Feed(sample);

  if (TakeDebugRequest())
  {
    if (reduced)
    {
      SetState(StringUtils::Format("{} (from {}x{} reductions)", m_selector.Describe(),
                                   m_reduction.width, m_reduction.height));
    }
    else
    {
      SetState(m_selector.Describe());
    }
  }

  if (served)
  {
    CLog::LogF(LOGDEBUG, "live content geometry now {}x{} at {},{}{}", served->rect.Width(),
               served->rect.Height(), served->rect.x1, served->rect.y1,
               served->varies ? " (varies)" : "");
    Post({.rect = served->rect, .varies = served->varies});
  }

  return InForce();
}

bool CLiveGeometryMonitor::AcquireFrame(const VideoPicture& picture,
                                        const CDVDStreamInfo& hints,
                                        FrameRef& frame,
                                        bool& reduced)
{
  reduced = false;
  if (CVideoFileGeometry::BuildGeometryFrameRef(picture, hints, frame))
    return true;

  ReductionResult result = ReductionResult::Unsupported;
  if (picture.videoBuffer)
  {
    result = picture.videoBuffer->ReduceForAnalysis(m_reduction, picture.iWidth, picture.iHeight,
                                                    REDUCTION_WIDTH);
    if (result == ReductionResult::Produced)
      reduced = CVideoFileGeometry::BuildGeometryFrameRef(m_reduction, picture, hints, frame);
  }

  if (!reduced && result == ReductionResult::Pending)
  {
    SetState("waiting for the readback");
    return false;
  }

  if (!reduced)
  {
    if (!m_unreadableLogged)
    {
      m_unreadableLogged = true;
      CLog::LogF(LOGINFO, "live content geometry: frames not readable ({})",
                 PixelFormatName(picture.videoBuffer ? picture.videoBuffer->GetFormat()
                                                     : picture.pixelFormat));
    }
    SetState("unavailable: decoded frames are not readable");
    return false;
  }

  if (!m_reducedLogged)
  {
    m_reducedLogged = true;
    CLog::LogF(LOGINFO, "live content geometry: reading {}x{} reductions ({})", m_reduction.width,
               m_reduction.height, PixelFormatName(picture.pixelFormat));
  }

  return true;
}

CRectInt CLiveGeometryMonitor::InForce() const
{
  return m_selector.Published().value_or(CRectInt{});
}

std::string CLiveGeometryMonitor::GetDebugInfo() const
{
  m_debugReads.fetch_add(1, std::memory_order_relaxed);

  std::unique_lock lock(m_section);
  return m_state;
}

bool CLiveGeometryMonitor::TakeDebugRequest()
{
  const uint32_t reads = m_debugReads.load(std::memory_order_relaxed);
  const bool wanted = reads != m_debugReadsSeen;
  m_debugReadsSeen = reads;
  return wanted;
}

void CLiveGeometryMonitor::Post(const LiveGeometryUpdate& update)
{
  m_messageParent.Put(
      std::make_shared<CDVDMsgType<LiveGeometryUpdate>>(CDVDMsg::PLAYER_CONTENT_GEOMETRY, update));
}

void CLiveGeometryMonitor::SetState(std::string_view state)
{
  if (m_stateWritten == state)
    return;

  m_stateWritten = state;

  std::unique_lock lock(m_section);
  m_state = m_stateWritten;
}
