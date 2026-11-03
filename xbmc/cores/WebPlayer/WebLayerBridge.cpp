/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebLayerBridge.h"

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "utils/log.h"
#include "windowing/gbm/drm/DRMAtomic.h"

#include <mutex>
#include <utility>

using namespace KODI::WEB;
using namespace KODI::WINDOWING::GBM;

CWebLayerBridge::~CWebLayerBridge()
{
  for (CVideoBufferDRMPRIME* buffer : {m_pending, m_page, m_prevPage, m_video})
  {
    if (buffer)
      buffer->Release();
  }

  // the plane-off commit from Disable has run by now
  for (uint32_t fbId : m_fbCache.TakeAll())
    drmModeRmFB(m_DRM->GetFileDescriptor(), fbId);
}

void CWebLayerBridge::SetPage(CVideoBufferDRMPRIME* page)
{
  std::unique_lock lock(m_lock);
  if (m_pending)
    m_pending->Release();
  m_pending = page;
  m_clear = false;
}

void CWebLayerBridge::ClearPage()
{
  std::unique_lock lock(m_lock);
  if (m_pending)
    m_pending->Release();
  m_pending = nullptr;
  m_clear = true;
}

void CWebLayerBridge::SetVideoRect(const CRect& rect)
{
  std::unique_lock lock(m_lock);
  if (rect == m_videoRect)
    return;
  m_videoRect = rect;
  m_videoRectChanged = true;
}

void CWebLayerBridge::Disable()
{
  CVideoLayerBridgeDRMPRIME::Disable();
  std::unique_lock lock(m_lock);
  m_videoShown = false;
  HidePage();
}

void CWebLayerBridge::SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect)
{
  if (buffer != m_video)
  {
    buffer->Acquire();
    if (m_video)
      m_video->Release();
    m_video = buffer;
  }

  CVideoLayerBridgeDRMPRIME::SetVideoPlane(buffer, FitVideo(buffer->GetPicture(), destRect));
  ApplyPage();
}

CRect CWebLayerBridge::FitVideo(const VideoPicture& picture, const CRect& destRect)
{
  std::unique_lock lock(m_lock);
  m_rendererRect = destRect;
  m_videoAspect = picture.iDisplayHeight ? static_cast<float>(picture.iDisplayWidth) /
                                               static_cast<float>(picture.iDisplayHeight)
                                         : 0.0f;
  m_videoRectChanged = false;
  m_videoShown = true;
  return VideoDest();
}

// a moved video goes through SetVideoPlane() like a new frame, to be described in full
void CWebLayerBridge::UpdateVideoPlane()
{
  const std::optional<CRect> moved = MovedVideo();
  if (moved)
    CVideoLayerBridgeDRMPRIME::SetVideoPlane(m_video, *moved);
  else
    CVideoLayerBridgeDRMPRIME::UpdateVideoPlane();
  ApplyPage();
}

// the page's rectangle in screen pixels, fitted to the video's aspect ratio
CRect CWebLayerBridge::VideoDest() const
{
  // without a page over it, the video plane carries the page itself
  const CVideoBufferDRMPRIME* page = m_clear ? nullptr : m_pending ? m_pending : m_page;
  if (m_videoRect.IsEmpty() || !page)
    return m_rendererRect;

  const RESOLUTION_INFO mode = m_DRM->GetCurrentMode();
  const float screenWidth = static_cast<float>(mode.iScreenWidth);
  const float screenHeight = static_cast<float>(mode.iScreenHeight);
  const float scaleX = screenWidth / static_cast<float>(page->GetWidth());
  const float scaleY = screenHeight / static_cast<float>(page->GetHeight());
  CRect dest(m_videoRect.x1 * scaleX, m_videoRect.y1 * scaleY, m_videoRect.x2 * scaleX,
             m_videoRect.y2 * scaleY);

  if (m_videoAspect <= 0.0f)
    return dest;

  const float width = dest.Width();
  const float height = dest.Height();
  if (width > height * m_videoAspect)
  {
    const float fitted = height * m_videoAspect;
    dest.x1 += (width - fitted) / 2;
    dest.x2 = dest.x1 + fitted;
  }
  else
  {
    const float fitted = width / m_videoAspect;
    dest.y1 += (height - fitted) / 2;
    dest.y2 = dest.y1 + fitted;
  }
  return dest;
}

std::optional<CRect> CWebLayerBridge::MovedVideo()
{
  std::unique_lock lock(m_lock);
  if (!m_video || !m_videoShown || !m_videoRectChanged)
    return {};

  m_videoRectChanged = false;
  return VideoDest();
}

void CWebLayerBridge::ApplyPage()
{
  std::unique_lock lock(m_lock);

  if (m_clear)
  {
    m_clear = false;
    HidePage();
    m_plane = nullptr;
    m_stackedOn = nullptr;
    if (m_prevPage)
      m_prevPage->Release();
    m_prevPage = std::exchange(m_page, nullptr);
    m_prevPageFb = std::exchange(m_pageFb, 0);
    return;
  }

  if (m_pending)
    ShowPage();

  if (!m_page || !StackPage())
    return;

  const RESOLUTION_INFO mode = m_DRM->GetCurrentMode();
  m_DRM->AddProperty(m_plane, "FB_ID", m_pageFb);
  m_DRM->AddProperty(m_plane, "CRTC_ID", m_DRM->GetCrtc()->GetCrtcId());
  m_DRM->AddProperty(m_plane, "SRC_X", 0);
  m_DRM->AddProperty(m_plane, "SRC_Y", 0);
  m_DRM->AddProperty(m_plane, "SRC_W", m_page->GetWidth() << 16);
  m_DRM->AddProperty(m_plane, "SRC_H", m_page->GetHeight() << 16);
  m_DRM->AddProperty(m_plane, "CRTC_X", 0);
  m_DRM->AddProperty(m_plane, "CRTC_Y", 0);
  m_DRM->AddProperty(m_plane, "CRTC_W", mode.iScreenWidth);
  m_DRM->AddProperty(m_plane, "CRTC_H", mode.iScreenHeight);
}

void CWebLayerBridge::ShowPage()
{
  CVideoBufferDRMPRIME* page = std::exchange(m_pending, nullptr);
  AVDRMFrameDescriptor* descriptor = page->GetDescriptor();

  const auto identity =
      DRMPRIME::GetDmaBufIdentity(descriptor, page->GetWidth(), page->GetHeight());
  uint32_t fbId = identity ? m_fbCache.Lookup(*identity) : 0;
  if (!fbId)
  {
    fbId = CreateFramebuffer(page);
    if (!fbId)
    {
      page->Release();
      return;
    }
    if (identity)
      m_fbCache.Insert(*identity, fbId);
  }

  if (m_prevPage)
    m_prevPage->Release();
  m_prevPage = std::exchange(m_page, page);
  m_prevPageFb = std::exchange(m_pageFb, fbId);

  for (uint32_t doomed : m_fbCache.Reap({m_pageFb, m_prevPageFb}))
    drmModeRmFB(m_DRM->GetFileDescriptor(), doomed);
}

// the renderer picks the video plane anew for each new video format
bool CWebLayerBridge::StackPage()
{
  CDRMPlane* video = m_DRM->GetVideoPlane();
  if (m_stackedOn == video)
    return m_plane != nullptr;

  HidePage();
  const AVDRMFrameDescriptor* descriptor = m_page->GetDescriptor();
  const uint32_t format = descriptor->layers[0].format;
  const uint64_t modifier = descriptor->objects[0].format_modifier;
  m_plane = m_DRM->FindOverlayPlane(format, modifier, m_page->GetWidth(), m_page->GetHeight());
  m_stackedOn = video;
  if (!m_plane)
    CLog::Log(LOGERROR, "CWebLayerBridge::{}: no plane between the video and the GUI plane",
              __FUNCTION__);
  return m_plane != nullptr;
}

void CWebLayerBridge::HidePage()
{
  if (!m_plane)
    return;

  m_DRM->AddProperty(m_plane, "FB_ID", 0);
  m_DRM->AddProperty(m_plane, "CRTC_ID", 0);
}
