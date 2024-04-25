/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DebugRenderer.h"

#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayLibass.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "settings/SubtitlesSettings.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

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
}

void CDebugRenderer::Dispose()
{
  m_isInitialized = false;
  m_overlayRenderer.Flush();
  m_overlay.reset();
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

      std::shared_ptr<COverlay> o =
          ConvertLibass(*ovAss, it->pts, updateStyle, m_debugOverlayStyle);

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
