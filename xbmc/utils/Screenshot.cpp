/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Screenshot.h"

#include "system.h"
#include <vector>

#include "Util.h"

#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "pictures/Picture.h"

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

#include "filesystem/File.h"
#include "guilib/GraphicContext.h"

#include "utils/JobManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "settings/GUISettings.h"

using namespace std;
using namespace XFILE;

CScreenshotSurface::CScreenshotSurface()
{
  m_width = 0;
  m_height = 0;
  m_stride = 0;
  m_buffer = NULL;
}

bool CScreenshotSurface::capture()
{

#ifdef HAS_DX
  LPDIRECT3DSURFACE9 lpSurface = NULL, lpBackbuffer = NULL;
  g_graphicsContext.Lock();
  if (g_application.IsPlayingVideo())
  {
#ifdef HAS_VIDEO_PLAYBACK
    g_renderManager.SetupScreenshot();
#endif
  }
  g_application.RenderNoPresent();

  if (FAILED(g_Windowing.Get3DDevice()->CreateOffscreenPlainSurface(g_Windowing.GetWidth(), g_Windowing.GetHeight(), D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &lpSurface, NULL)))
    return false;

  if (FAILED(g_Windowing.Get3DDevice()->GetRenderTarget(0, &lpBackbuffer)))
    return false;

  // now take screenshot
  if (SUCCEEDED(g_Windowing.Get3DDevice()->GetRenderTargetData(lpBackbuffer, lpSurface)))
  {
    D3DLOCKED_RECT lr;
    D3DSURFACE_DESC desc;
    lpSurface->GetDesc(&desc);
    if (SUCCEEDED(lpSurface->LockRect(&lr, NULL, D3DLOCK_READONLY)))
    {
      m_width = desc.Width;
      m_height = desc.Height;
      m_stride = lr.Pitch;
      m_buffer = new unsigned char[m_height * m_stride];
      memcpy(m_buffer, lr.pBits, m_height * m_stride);
      lpSurface->UnlockRect();
    }
    else
    {
      CLog::Log(LOGERROR, "%s LockRect failed", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s GetBackBuffer failed", __FUNCTION__);
  }
  lpSurface->Release();
  lpBackbuffer->Release();

  g_graphicsContext.Unlock();

#elif defined(HAS_GL) || defined(HAS_GLES)

  g_graphicsContext.BeginPaint();
  if (g_application.IsPlayingVideo())
  {
#ifdef HAS_VIDEO_PLAYBACK
    g_renderManager.SetupScreenshot();
#endif
  }
  g_application.RenderNoPresent();
#ifndef HAS_GLES
  glReadBuffer(GL_BACK);
#endif
  //get current viewport
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  m_width  = viewport[2] - viewport[0];
  m_height = viewport[3] - viewport[1];
  m_stride = m_width * 4;
  unsigned char* surface = new unsigned char[m_stride * m_height];

  //read pixels from the backbuffer
#if HAS_GLES == 2
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#else
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#endif
  g_graphicsContext.EndPaint();

  //make a new buffer and copy the read image to it with the Y axis inverted
  m_buffer = new unsigned char[m_stride * m_height];
  for (int y = 0; y < m_height; y++)
  {
#ifdef HAS_GLES
    // we need to save in BGRA order so XOR Swap RGBA -> BGRA
    unsigned char* swap_pixels = surface + (m_height - y - 1) * m_stride;
    for (int x = 0; x < m_width; x++, swap_pixels+=4)
    {
      std::swap(swap_pixels[0], swap_pixels[2]);
    }   
#endif
    memcpy(m_buffer + y * m_stride, surface + (m_height - y - 1) *m_stride, m_stride);
  }

  delete [] surface;

#else
  //nothing to take a screenshot from
  return false;
#endif

  return true;
}

void CScreenShot::TakeScreenshot(const CStdString &filename, bool sync)
{

  CScreenshotSurface surface;
  if (!surface.capture())
  {
    CLog::Log(LOGERROR, "Screenshot %s failed", filename.c_str());
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot %s", filename.c_str());

  //set alpha byte to 0xFF
  for (int y = 0; y < surface.m_height; y++)
  {
    unsigned char* alphaptr = surface.m_buffer - 1 + y * surface.m_stride;
    for (int x = 0; x < surface.m_width; x++)
      *(alphaptr += 4) = 0xFF;
  }

  //if sync is true, the png file needs to be completely written when this function returns
  if (sync)
  {
    if (!CPicture::CreateThumbnailFromSurface(surface.m_buffer, surface.m_width, surface.m_height, surface.m_stride, filename))
      CLog::Log(LOGERROR, "Unable to write screenshot %s", filename.c_str());

    delete [] surface.m_buffer;
  }
  else
  {
    //make sure the file exists to avoid concurrency issues
    FILE* fp = fopen(filename.c_str(), "w");
    if (fp)
      fclose(fp);
    else
      CLog::Log(LOGERROR, "Unable to create file %s", filename.c_str());

    //write .png file asynchronous with CThumbnailWriter, prevents stalling of the render thread
    //buffer is deleted from CThumbnailWriter
    CThumbnailWriter* thumbnailwriter = new CThumbnailWriter(surface.m_buffer, surface.m_width, surface.m_height, surface.m_stride, filename);
    CJobManager::GetInstance().AddJob(thumbnailwriter, NULL);
  }
}

void CScreenShot::TakeScreenshot()
{
  static bool savingScreenshots = false;
  static vector<CStdString> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("debug.screenshotpath", false);
  if (strDir.IsEmpty())
  {
    strDir = "special://temp/";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot%03d.png"), 999);

    if (!file.IsEmpty())
    {
      TakeScreenshot(file, false);
      if (savingScreenshots)
        screenShots.push_back(file);
      if (promptUser)
      { // grab the real directory
        CStdString newDir = g_guiSettings.GetString("debug.screenshotpath");
        if (!newDir.IsEmpty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            CStdString file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(newDir, "screenshot%03d.png"), 999);
            CFile::Cache(screenShots[i], file);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}
