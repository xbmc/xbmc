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

#include "system.h"
#include "Splash.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIFontManager.h"
#include "filesystem/File.h"
#include "windowing/WindowingFactory.h"
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const std::string& imageName)
{
  m_imageName = imageName;
  m_fade = 0.5;
  m_messageLayout = NULL;
  m_image = NULL;
  m_layoutWasLoading = false;
}


CSplash::~CSplash()
{
  delete m_image;
  delete m_messageLayout;
}

void CSplash::Show()
{
  Show("");
}

void CSplash::Show(const std::string& message)
{
  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  g_graphicsContext.SetRenderingResolution(res, true);

  if (!m_image)
  {
    m_image = new CGUIImage(0, 0, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), CTextureInfo(m_imageName));
    m_image->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  //render splash image
  g_Windowing.BeginRender();

  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();

  // render message
  if (!message.empty())
  {
    if (!m_layoutWasLoading)
    {
      // load arial font, white body, no shadow, size: 20, no additional styling

      CGUIFont *messageFont = g_fontManager.LoadTTF("__splash__", "arial.ttf", 0xFFFFFFFF, 0, 20, FONT_STYLE_NORMAL, false, 1.0f, 1.0f, &res);
      if (messageFont)
        m_messageLayout = new CGUITextLayout(messageFont, true, 0);
      m_layoutWasLoading = true;
    }
    if (m_messageLayout)
    {
      m_messageLayout->Update(message, 1150, false, true);

      float textWidth, textHeight;
      m_messageLayout->GetTextExtent(textWidth, textHeight);
      // ideally place text in center of empty area below splash image
      float y = 540 + m_image->GetTextureHeight() / 4 - textHeight / 2;
      if (y + textHeight > 720) // make sure entire text is visible
        y = 720 - textHeight;

      m_messageLayout->RenderOutline(640, y, 0, 0xFF000000, XBFONT_CENTER_X, 1280);
    }
  }

  //show it on screen
  g_Windowing.EndRender();
  CDirtyRegionList dirty;
  g_graphicsContext.Flip(dirty);
  g_graphicsContext.Unlock();
}
