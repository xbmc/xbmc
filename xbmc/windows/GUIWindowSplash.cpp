/*
 *      Copyright (C) 2015 Team XBMC
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

#include "GUIWindowSplash.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

CGUIWindowSplash::CGUIWindowSplash(void) : CGUIWindow(WINDOW_SPLASH, "")
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_image = nullptr;
}

CGUIWindowSplash::~CGUIWindowSplash(void)
{

}

void CGUIWindowSplash::OnInitWindow()
{
  std::string splashImage = "special://home/media/Splash.png";
  if (!XFILE::CFile::Exists(splashImage))
    splashImage = "special://xbmc/media/Splash.png";

  CLog::Log(LOGINFO, "load splash image: %s", CSpecialProtocol::TranslatePath(splashImage).c_str());

  m_image = std::unique_ptr<CGUIImage>(new CGUIImage(0, 0, 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), CTextureInfo(splashImage)));
  m_image->SetAspectRatio(CAspectRatio::AR_SCALE);
}

void CGUIWindowSplash::Render()
{
  g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), true);
  m_image->SetWidth(g_graphicsContext.GetWidth());
  m_image->SetHeight(g_graphicsContext.GetHeight());
  m_image->AllocResources();
  m_image->Render();
  m_image->FreeResources();
}
