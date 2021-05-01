/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "DebugRenderer.h"

#include "OverlayRendererGUI.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayText.h"
#include "windowing/GraphicContext.h"

using namespace OVERLAY;

CDebugRenderer::CDebugRenderer()
{
  for (int i = 0; i < 6; i++)
  {
    m_overlay[i] = nullptr;
    m_strDebug[i] = " ";
  }
}

CDebugRenderer::~CDebugRenderer()
{
  for (CDVDOverlayText* overlayText : m_overlay)
  {
    if (overlayText)
      overlayText->Release();
  }
}

void CDebugRenderer::SetInfo(DEBUG_INFO_PLAYER& info)
{
  m_overlayRenderer.Release(0);

  if (info.audio != m_strDebug[0])
  {
    m_strDebug[0] = info.audio;
    if (m_overlay[0])
      m_overlay[0]->Release();
    m_overlay[0] = new CDVDOverlayText();
    m_overlay[0]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[0]));
  }
  if (info.video != m_strDebug[1])
  {
    m_strDebug[1] = info.video;
    if (m_overlay[1])
      m_overlay[1]->Release();
    m_overlay[1] = new CDVDOverlayText();
    m_overlay[1]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[1]));
  }
  if (info.player != m_strDebug[2])
  {
    m_strDebug[2] = info.player;
    if (m_overlay[2])
      m_overlay[2]->Release();
    m_overlay[2] = new CDVDOverlayText();
    m_overlay[2]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[2]));
  }
  if (info.vsync != m_strDebug[3])
  {
    m_strDebug[3] = info.vsync;
    if (m_overlay[3])
      m_overlay[3]->Release();
    m_overlay[3] = new CDVDOverlayText();
    m_overlay[3]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[3]));
  }

  for (int i = 0; i < 4; i++)
    m_overlayRenderer.AddOverlay(m_overlay[i], 0, 0);
}

void CDebugRenderer::SetInfo(DEBUG_INFO_VIDEO& video, DEBUG_INFO_RENDER& render)
{
  m_overlayRenderer.Release(0);

  if (video.videoSource != m_strDebug[0])
  {
    m_strDebug[0] = video.videoSource;
    if (m_overlay[0])
      m_overlay[0]->Release();
    m_overlay[0] = new CDVDOverlayText();
    m_overlay[0]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[0]));
  }
  if (video.metaPrim != m_strDebug[1])
  {
    m_strDebug[1] = video.metaPrim;
    if (m_overlay[1])
      m_overlay[1]->Release();
    m_overlay[1] = new CDVDOverlayText();
    m_overlay[1]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[1]));
  }
  if (video.metaLight != m_strDebug[2])
  {
    m_strDebug[2] = video.metaLight;
    if (m_overlay[2])
      m_overlay[2]->Release();
    m_overlay[2] = new CDVDOverlayText();
    m_overlay[2]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[2]));
  }
  if (video.shader != m_strDebug[3])
  {
    m_strDebug[3] = video.shader;
    if (m_overlay[3])
      m_overlay[3]->Release();
    m_overlay[3] = new CDVDOverlayText();
    m_overlay[3]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[3]));
  }
  if (render.renderFlags != m_strDebug[4])
  {
    m_strDebug[4] = render.renderFlags;
    if (m_overlay[4])
      m_overlay[4]->Release();
    m_overlay[4] = new CDVDOverlayText();
    m_overlay[4]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[4]));
  }
  if (render.videoOutput != m_strDebug[5])
  {
    m_strDebug[5] = render.videoOutput;
    if (m_overlay[5])
      m_overlay[5]->Release();
    m_overlay[5] = new CDVDOverlayText();
    m_overlay[5]->AddElement(new CDVDOverlayText::CElementText(m_strDebug[5]));
  }

  for (int i = 0; i < 6; i++)
    m_overlayRenderer.AddOverlay(m_overlay[i], 0, 0);
}

void CDebugRenderer::Render(CRect& src, CRect& dst, CRect& view)
{
  m_overlayRenderer.SetVideoRect(src, dst, view);
  m_overlayRenderer.Render(0);
}

void CDebugRenderer::Flush()
{
  m_overlayRenderer.Flush();
}

CDebugRenderer::CRenderer::CRenderer() : OVERLAY::CRenderer()
{
  m_font = "__debug__";
  m_fontBorder = "__debugborder__";
}

void CDebugRenderer::CRenderer::Render(int idx)
{
  std::vector<COverlay*> render;
  std::vector<SElement>& list = m_buffers[idx];
  float posY = 0.0f;
  for (std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    COverlay* o = nullptr;

    if (it->overlay_dvd)
      o = Convert(it->overlay_dvd, it->pts);

    if (!o)
      continue;

    COverlayText *text = dynamic_cast<COverlayText*>(o);
    if (text)
      text->PrepareRender("arial.ttf", 1, 100, 15, 0, m_font, m_fontBorder, UTILS::COLOR::NONE,
                          m_rv);

    RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());

    o->m_pos = COverlay::POSITION_ABSOLUTE;
    o->m_align = COverlay::ALIGN_SCREEN;
    o->m_x = 10 + (o->m_width * m_rv.Width() / res.iWidth) / 2;
    o->m_y = posY + o->m_height;
    OVERLAY::CRenderer::Render(o, 0);

    posY = o->m_y;
  }

  ReleaseUnused();
}
