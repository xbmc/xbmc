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

CSplash::CSplash()
  : m_image(nullptr)
{
}

CSplash::~CSplash()
{
  delete m_image;
}

CSplash& CSplash::GetInstance()
{
  static CSplash instance;
  return instance;
}

void CSplash::Show()
{
  if (!m_image)
  {
    std::string splashImage = "special://home/media/Splash.png";
    if (!XFILE::CFile::Exists(splashImage))
      splashImage = "special://xbmc/media/Splash.png";

    m_image = new CGUIImage(0, 0, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), CTextureInfo(splashImage));
    m_image->SetAspectRatio(CAspectRatio::AR_SCALE);
  }

  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  g_graphicsContext.SetRenderingResolution(res, true);

  //render splash image
  g_Windowing.BeginRender();

  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();

  //show it on screen
  g_Windowing.EndRender();
  g_graphicsContext.Flip(true, false);
  g_graphicsContext.Unlock();
}
