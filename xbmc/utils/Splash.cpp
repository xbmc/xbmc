/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "Splash.h"
#include "guilib/GUIImage.h"
#include "filesystem/File.h"
#include "windowing/WindowingFactory.h"
#include "rendering/RenderSystem.h"
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName)
{
  m_ImageName = imageName;
  fade = 0.5;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Show()
{
  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  g_graphicsContext.SetRenderingResolution(RES_HDTV_720p, true);  
  CGUIImage* image = new CGUIImage(0, 0, 0, 0, 1280, 720, m_ImageName);  
  image->SetAspectRatio(CAspectRatio::AR_CENTER);  
  image->AllocResources();

  //render splash image
  g_Windowing.BeginRender();

  image->Render();
  image->FreeResources();
  delete image;

  //show it on screen
  g_Windowing.EndRender();
  g_graphicsContext.Flip();
  g_graphicsContext.Unlock();
}

void CSplash::Hide()
{
}

void CSplash::Process()
{
  Show();
}

bool CSplash::Start()
{
  if (m_ImageName.IsEmpty() || !CFile::Exists(m_ImageName))
  {
    CLog::Log(LOGDEBUG, "Splash image %s not found", m_ImageName.c_str());
    return false;
  }
  Create();
  return true;
}

void CSplash::Stop()
{
  StopThread();
}

bool CSplash::IsRunning()
{
  return (m_ThreadHandle != NULL);
}
