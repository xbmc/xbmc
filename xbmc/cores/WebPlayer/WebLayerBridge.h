/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/DmaBufIdentityCache.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/VideoLayerBridgeDRMPRIME.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

#include <optional>

struct VideoPicture;

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{
class CDRMPlane;
}
} // namespace WINDOWING

namespace WEB
{

/*!
 * \brief The video layer bridge of the web player. While a page's video is
 * on the video plane, the page itself is on a plane between the video and
 * the GUI plane, where it covers the video except where it is transparent.
 */
class CWebLayerBridge : public CVideoLayerBridgeDRMPRIME
{
public:
  using CVideoLayerBridgeDRMPRIME::CVideoLayerBridgeDRMPRIME;
  ~CWebLayerBridge() override;

  void Disable() override;
  void SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect) override;
  void UpdateVideoPlane() override;

  /*!
   * \brief Show the page on the overlay plane from the next flip on; takes
   * over the caller's reference to the page; any thread
   */
  void SetPage(CVideoBufferDRMPRIME* page);

  /*!
   * \brief Take the page off the overlay plane at the next flip; any thread
   */
  void ClearPage();

  /*!
   * \brief Show the video at a rectangle of the page, in the page's pixels; an
   * empty one leaves the placement to the renderer; any thread
   */
  void SetVideoRect(const CRect& rect);

private:
  CRect FitVideo(const VideoPicture& picture, const CRect& destRect);
  CRect VideoDest() const;
  std::optional<CRect> MovedVideo();
  void ApplyPage();
  void ShowPage();
  bool StackPage();
  void HidePage();

  CVideoBufferDRMPRIME* m_video{nullptr};

  CCriticalSection m_lock;
  CVideoBufferDRMPRIME* m_pending{nullptr};
  bool m_clear{false};
  CVideoBufferDRMPRIME* m_page{nullptr};
  CVideoBufferDRMPRIME* m_prevPage{nullptr};
  uint32_t m_pageFb{0};
  uint32_t m_prevPageFb{0};
  KODI::WINDOWING::GBM::CDRMPlane* m_plane{nullptr};
  KODI::WINDOWING::GBM::CDRMPlane* m_stackedOn{nullptr};
  CRect m_videoRect;
  CRect m_rendererRect;
  float m_videoAspect{0.0f};
  bool m_videoRectChanged{false};
  bool m_videoShown{false};
  DRMPRIME::CDmaBufIdentityCache m_fbCache{MAX_FB_CACHE, "web-page-fb"};
};

} // namespace WEB
} // namespace KODI
