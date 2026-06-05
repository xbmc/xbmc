/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DebugRenderer.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayLibass.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "settings/SubtitlesSettings.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

using namespace OVERLAY;

CDebugRenderer::CDebugRenderer()
{
}

CDebugRenderer::~CDebugRenderer()
{
  Dispose();
}

void CDebugRenderer::Initialize()
{
  if (m_isInitialized)
    return;

  m_adapter = new CSubtitlesAdapter();

  m_isInitialized = m_adapter->Initialize();
  if (!m_isInitialized)
  {
    CLog::Log(LOGERROR, "{} - Failed to configure OSD info debug renderer", __FUNCTION__);
    delete m_adapter;
    m_adapter = nullptr;
    return;
  }

  // We create only a single overlay with a fixed PTS for each rendered frame
  m_overlay = m_adapter->CreateOverlay();
  m_overlayRenderer.AddOverlay(m_overlay, 1000000., 0);

  // Kick the dirty-driven walk skip so the toggle takes effect this frame;
  // without this the walk stays idle and the debug overlay never starts
  // rendering until something else dirties the GUI.
  OVERLAY::MarkDirty();
}

void CDebugRenderer::Dispose()
{
  m_isInitialized = false;
  m_overlayRenderer.Flush();
  m_overlay.reset();
  OVERLAY::MarkDirty();
  if (m_adapter)
  {
    delete m_adapter;
    m_adapter = nullptr;
  }
}

void CDebugRenderer::SetInfo(DEBUG_INFO_PLAYER& info)
{
  if (!m_isInitialized)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.
  m_adapter->FlushSubtitles();
  m_adapter->AddSubtitle(info.audio, 0., 5000000.);
  m_adapter->AddSubtitle(info.video, 0., 5000000.);
  m_adapter->AddSubtitle(info.player, 0., 5000000.);
  m_adapter->AddSubtitle(info.vsync, 0., 5000000.);
}

void CDebugRenderer::SetInfo(DEBUG_INFO_VIDEO& video, DEBUG_INFO_RENDER& render)
{
  if (!m_isInitialized)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.
  m_adapter->FlushSubtitles();
  m_adapter->AddSubtitle(video.videoSource, 0., 5000000.);
  m_adapter->AddSubtitle(video.metaPrim, 0., 5000000.);
  m_adapter->AddSubtitle(video.metaLight, 0., 5000000.);
  m_adapter->AddSubtitle(video.shader, 0., 5000000.);
  m_adapter->AddSubtitle(video.render, 0., 5000000.);
  m_adapter->AddSubtitle(render.renderFlags, 0., 5000000.);
  m_adapter->AddSubtitle(render.videoOutput, 0., 5000000.);
}

void CDebugRenderer::Render(CRect& src, CRect& dst, CRect& view)
{
  if (!m_isInitialized)
    return;

  m_overlayRenderer.SetVideoRect(src, dst, view);
  m_overlayRenderer.Render(0);

  // Sustain the dirty-driven Render skip while the debug OSD is enabled.
  // Unlike video subtitles (which call AddOverlay per video frame and so
  // mark dirty per frame), the debug OSD only calls AddOverlay once at
  // Initialize, so without a per-frame nudge here the GUI walk would not
  // re-fire after the first frame and the debug OSD would freeze.
  MarkDirty();
}

void CDebugRenderer::Flush()
{
  if (!m_isInitialized)
    return;

  m_adapter->FlushSubtitles();
}

CDebugRenderer::CRenderer::CRenderer() : OVERLAY::CRenderer()
{
}

void CDebugRenderer::CRenderer::Render(int idx, float depth)
{
  std::vector<SElement>& list = m_buffers[idx];
  for (std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    if (it->overlay_dvd)
    {
      auto ovAss = std::static_pointer_cast<CDVDOverlayLibass>(it->overlay_dvd);
      if (!ovAss || !ovAss->GetLibassHandler())
        continue;

      bool updateStyle = !m_debugOverlayStyle;
      if (updateStyle)
        CreateSubtitlesStyle();

      // Debug OSD is a standalone path: text content changes every frame
      // and DebugRenderer::Render MarkDirty's per frame anyway, so it does
      // not participate in the PrepareOverlays / SElement caching scheme.
      // The rOpts setup below mirrors what master's CRenderer::ConvertLibass
      // did inline.
      KODI::SUBTITLES::STYLE::renderOpts rOpts;
      rOpts.sourceWidth = m_rs.Width();
      rOpts.sourceHeight = m_rs.Height();
      rOpts.videoWidth = m_rd.Width();
      rOpts.videoHeight = m_rd.Height();
      rOpts.frameWidth = m_rv.Width();
      rOpts.frameHeight = m_rv.Height();

      // Render subtitle of half-sbs and half-ou video in full screen, not in half screen
      if (m_stereomode == "left_right" || m_stereomode == "right_left")
      {
        // only half-sbs video, sbs video don't need to change source size
        if (rOpts.sourceWidth / rOpts.sourceHeight < 1.2f)
          rOpts.sourceWidth = m_rs.Width() * 2;
      }
      else if (m_stereomode == "top_bottom" || m_stereomode == "bottom_top")
      {
        // only half-ou video, ou video don't need to change source size
        if (rOpts.sourceWidth / rOpts.sourceHeight > 2.5f)
          rOpts.sourceHeight = m_rs.Height() * 2;
      }

      // Set position of subtitles based on video calibration settings
      RESOLUTION_INFO resInfo = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
      // Keep track of subtitle position value change,
      // can be changed by GUI Calibration or by window mode/resolution change or
      // by user manual change (e.g. keyboard shortcut)
      if (m_subtitlePosResInfo != resInfo.iSubtitles)
      {
        if (m_subtitlePosResInfo == POSRESINFO_SAVE_CHANGES)
        {
          // m_subtitlePosition has been changed
          // and has been requested to save the value to resInfo
          resInfo.iSubtitles = m_subtitlePosition + m_subtitleVerticalMargin;
          CServiceBroker::GetWinSystem()->GetGfxContext().SetResInfo(
              CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), resInfo);
          m_subtitlePosResInfo = m_subtitlePosition + m_subtitleVerticalMargin;
        }
        else
          ResetSubtitlePosition();
      }

      rOpts.m_par = resInfo.fPixelRatio;

      // rOpts.position and margins (set to style) can invalidate the text
      // positions to subtitles type that make use of margins to position text on
      // the screen (e.g. ASS/WebVTT) then we allow to set them when position
      // override setting is enabled only
      if (ovAss->IsForcedMargins())
      {
        rOpts.marginsMode = KODI::SUBTITLES::STYLE::MarginsMode::DISABLED;
      }
      else if (m_subtitleAlign == KODI::SUBTITLES::Align::MANUAL)
      {
        // When vertical margins are used Libass apply a displacement in percentage
        // of the height available to line position, this displacement causes
        // problems with subtitle calibration bar on Video Calibration window,
        // so when you moving the subtitle bar of the GUI the text will no longer
        // match the bar, this calculation compensates for the displacement.
        // Note also that the displacement compensation will cause a different
        // default position of the text, different from the other alignment positions
        double posPx = static_cast<double>(m_subtitlePosition - resInfo.Overscan.top);

        int assPlayResY = ovAss->GetLibassHandler()->GetPlayResY();
        double assVertMargin = static_cast<double>(m_debugOverlayStyle->marginVertical) *
                               (static_cast<double>(assPlayResY) / 720);
        double vertMarginScaled =
            assVertMargin / assPlayResY * static_cast<double>(rOpts.frameHeight);

        double pos = posPx / (static_cast<double>(rOpts.frameHeight) - vertMarginScaled);
        rOpts.position = 100 - pos * 100;
      }
      else if (m_subtitleAlign == KODI::SUBTITLES::Align::BOTTOM_OUTSIDE)
      {
        // To keep consistent the position of text as other alignment positions
        // we avoid apply the displacement compensation
        double posPx = static_cast<double>(m_subtitlePosition + m_subtitleVerticalMargin -
                                           resInfo.Overscan.top);
        rOpts.position = 100 - posPx / static_cast<double>(rOpts.frameHeight) * 100;
      }
      else if (m_subtitleAlign == KODI::SUBTITLES::Align::BOTTOM_INSIDE ||
               m_subtitleAlign == KODI::SUBTITLES::Align::TOP_INSIDE)
      {
        rOpts.marginsMode = KODI::SUBTITLES::STYLE::MarginsMode::INSIDE_VIDEO;
      }

      // Set the horizontal text alignment (currently used to improve readability on CC subtitles only)
      // This setting influence style->alignment property
      if (ovAss->IsTextAlignEnabled())
      {
        if (m_subtitleHorizontalAlign == KODI::SUBTITLES::HorizontalAlign::LEFT)
          rOpts.horizontalAlignment = KODI::SUBTITLES::STYLE::HorizontalAlign::LEFT;
        else if (m_subtitleHorizontalAlign == KODI::SUBTITLES::HorizontalAlign::RIGHT)
          rOpts.horizontalAlignment = KODI::SUBTITLES::STYLE::HorizontalAlign::RIGHT;
        else
          rOpts.horizontalAlignment = KODI::SUBTITLES::STYLE::HorizontalAlign::CENTER;
      }

      int changes = 0;
      ASS_Image* images = ovAss->GetLibassHandler()->RenderImage(it->pts, rOpts, updateStyle,
                                                                 m_debugOverlayStyle, &changes);

      if (!images)
        continue;

      std::shared_ptr<COverlay> o = COverlay::Create(images, rOpts.frameWidth, rOpts.frameHeight);

      if (o)
        OVERLAY::CRenderer::Render(o.get());
    }
  }
  ReleaseUnused();
}

void CDebugRenderer::CRenderer::CreateSubtitlesStyle()
{
  m_debugOverlayStyle = std::make_shared<KODI::SUBTITLES::STYLE::style>();
  m_debugOverlayStyle->fontName = KODI::SUBTITLES::FONT_DEFAULT_FAMILYNAME;
  m_debugOverlayStyle->fontSize = 20.0;
  m_debugOverlayStyle->marginVertical = 12;
}
