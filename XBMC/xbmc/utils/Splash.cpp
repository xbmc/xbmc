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
#include "GUIImage.h"
#include "FileSystem/File.h"
#include "WindowingFactory.h"
#include "RenderSystem.h"
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

  g_graphicsContext.SetCameraPosition(CPoint(0, 0));
  float w = g_graphicsContext.GetWidth() * 0.5f;
  float h = g_graphicsContext.GetHeight() * 0.5f;
  CGUIImage* image = new CGUIImage(0, 0, w*0.5f, h*0.5f, w, h, m_ImageName);
  image->SetAspectRatio(CAspectRatio::AR_KEEP);
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
