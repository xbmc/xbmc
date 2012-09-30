
#include "system.h"
#include <vector>
#include "Screenshot.h"

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

using namespace std;
using namespace XFILE;

CScreenshotSurface::CScreenshotSurface() {

  buffer = NULL;
}

boolean CScreenshotSurface::capture() {

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
      width = desc.Width;
      height = desc.Height;
      stride = lr.Pitch;
      buffer = new unsigned char[height * stride];
      memcpy(buffer, lr.pBits, height * stride);
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

  width  = viewport[2] - viewport[0];
  height = viewport[3] - viewport[1];
  stride = width * 4;
  unsigned char* surface = new unsigned char[stride * height];

  //read pixels from the backbuffer
#if HAS_GLES == 2
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#else
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#endif
  g_graphicsContext.EndPaint();

  //make a new buffer and copy the read image to it with the Y axis inverted
  buffer = new unsigned char[stride * height];
  for (int y = 0; y < height; y++)
  {
#ifdef HAS_GLES
    // we need to save in BGRA order so XOR Swap RGBA -> BGRA
    unsigned char* swap_pixels = surface + (height - y - 1) * stride;
    for (int x = 0; x < width; x++, swap_pixels+=4)
    {
      std::swap(swap_pixels[0], swap_pixels[2]);
    }   
#endif
    memcpy(buffer + y * stride, surface + (height - y - 1) * stride, stride);
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
  for (int y = 0; y < surface.height; y++)
  {
    unsigned char* alphaptr = surface.buffer - 1 + y * surface.stride;
    for (int x = 0; x < surface.width; x++)
      *(alphaptr += 4) = 0xFF;
  }

  //if sync is true, the png file needs to be completely written when this function returns
  if (sync)
  {
    if (!CPicture::CreateThumbnailFromSurface(surface.buffer, surface.width, surface.height, surface.stride, filename))
      CLog::Log(LOGERROR, "Unable to write screenshot %s", filename.c_str());

    delete [] surface.buffer;
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
    CThumbnailWriter* thumbnailwriter = new CThumbnailWriter(surface.buffer, surface.width, surface.height, surface.stride, filename);
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
