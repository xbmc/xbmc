/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderSystem.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIFontManager.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"

CRenderSystemBase::CRenderSystemBase()
  : m_stereoView(RENDER_STEREO_VIEW_OFF)
  , m_stereoMode(RENDER_STEREO_MODE_OFF)
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
  m_renderCaps = 0;
  m_renderQuirks = 0;
  m_minDXTPitch = 0;
}

CRenderSystemBase::~CRenderSystemBase() = default;

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor) const
{
  major = m_RenderVersionMajor;
  minor = m_RenderVersionMinor;
}

bool CRenderSystemBase::SupportsNPOT(bool dxt) const
{
  if (dxt)
    return (m_renderCaps & RENDER_CAPS_DXT_NPOT) == RENDER_CAPS_DXT_NPOT;
  return (m_renderCaps & RENDER_CAPS_NPOT) == RENDER_CAPS_NPOT;
}

bool CRenderSystemBase::SupportsDXT() const
{
  return (m_renderCaps & RENDER_CAPS_DXT) == RENDER_CAPS_DXT;
}

bool CRenderSystemBase::SupportsBGRA() const
{
  return (m_renderCaps & RENDER_CAPS_BGRA) == RENDER_CAPS_BGRA;
}

bool CRenderSystemBase::SupportsBGRAApple() const
{
  return (m_renderCaps & RENDER_CAPS_BGRA_APPLE) == RENDER_CAPS_BGRA_APPLE;
}

bool CRenderSystemBase::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch(mode)
  {
    case RENDER_STEREO_MODE_OFF:
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:
    case RENDER_STEREO_MODE_MONO:
      return true;
    default:
      return false;
  }
}

void CRenderSystemBase::ShowSplash(const std::string& message)
{
  if (!g_advancedSettings.m_splashImage && !(m_splashImage || !message.empty()))
    return;

  if (!m_splashImage)
  {
    std::string splashImage = "special://home/media/Splash.png";
    if (!XFILE::CFile::Exists(splashImage))
      splashImage = "special://xbmc/media/Splash.png";

    m_splashImage = std::unique_ptr<CGUIImage>(new CGUIImage(0, 0, 0, 0, g_graphicsContext.GetWidth(),
                                                       g_graphicsContext.GetHeight(), CTextureInfo(splashImage)));
    m_splashImage->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  g_graphicsContext.SetRenderingResolution(res, true);

  //render splash image
  BeginRender();

  m_splashImage->AllocResources();
  m_splashImage->Render();
  m_splashImage->FreeResources();

  if (!message.empty())
  {
    if (!m_splashMessageLayout)
    {
      auto messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 20, FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_splashMessageLayout = std::unique_ptr<CGUITextLayout>(new CGUITextLayout(messageFont, true, 0));
    }

    if (m_splashMessageLayout)
    {
      m_splashMessageLayout->Update(message, 1150, false, true);
      float textWidth, textHeight;
      m_splashMessageLayout->GetTextExtent(textWidth, textHeight);

      int width = g_graphicsContext.GetWidth();
      int height = g_graphicsContext.GetHeight();
      float y = height - textHeight - 100;
      m_splashMessageLayout->RenderOutline(width/2, y, 0, 0xFF000000, XBFONT_CENTER_X, width);
    }
  }

  //show it on screen
  EndRender();
  g_graphicsContext.Unlock();
  g_graphicsContext.Flip(true, false);
}

