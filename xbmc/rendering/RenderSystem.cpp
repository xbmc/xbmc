/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
#include "settings/AdvancedSettings.h"
#include "Util.h"

CRenderSystemBase::CRenderSystemBase()
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVersionMajor = 0;
  m_RenderVersionMinor = 0;
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
    return false;

  return true;
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
    m_splashImage = std::unique_ptr<CGUIImage>(new CGUIImage(0, 0, 0, 0, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                                                       CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(), CTextureInfo(CUtil::GetSplashPath())));
    m_splashImage->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Clear();

  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(res, true);

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

      int width = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
      int height = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
      float y = height - textHeight - 100;
      m_splashMessageLayout->RenderOutline(width/2, y, 0, 0xFF000000, XBFONT_CENTER_X, width);
    }
  }

  //show it on screen
  EndRender();
  CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(true, false);
}

