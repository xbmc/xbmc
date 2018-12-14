/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Screenshot.h"

#include "system_gl.h"
#include <vector>

#include "ServiceBroker.h"
#include "Util.h"
#include "URL.h"

#include "pictures/Picture.h"

#ifdef TARGET_RASPBERRY_PI
#include "platform/linux/RBP.h"
#endif

#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "windowing/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

#include "utils/JobManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "settings/SettingPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/windows/GUIControlSettings.h"

#if defined(HAS_LIBAMCODEC)
#include "utils/ScreenshotAML.h"
#endif

#if defined(TARGET_WINDOWS)
#include "rendering/dx/DeviceResources.h"
#include <wrl/client.h>
using namespace Microsoft::WRL;
#endif

using namespace XFILE;

CScreenshotSurface::CScreenshotSurface()
{
  m_width = 0;
  m_height = 0;
  m_stride = 0;
  m_buffer = NULL;
}

CScreenshotSurface::~CScreenshotSurface()
{
  delete m_buffer;
}

bool CScreenshotSurface::capture()
{
#if defined(TARGET_RASPBERRY_PI)
  g_RBP.GetDisplaySize(m_width, m_height);
  m_buffer = g_RBP.CaptureDisplay(m_width, m_height, &m_stride, true, false);
  if (!m_buffer)
    return false;
#elif defined(TARGET_WINDOWS)

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  CServiceBroker::GetGUI()->GetWindowManager().Render();

  auto deviceResources = DX::DeviceResources::Get();
  deviceResources->FinishCommandList();

  ComPtr<ID3D11DeviceContext> pImdContext = deviceResources->GetImmediateContext();
  ComPtr<ID3D11Device> pDevice = deviceResources->GetD3DDevice();
  CD3DTexture* backbuffer = deviceResources->GetBackBuffer();
  if (!backbuffer)
    return false;

  D3D11_TEXTURE2D_DESC desc = { 0 };
  backbuffer->GetDesc(&desc);
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.BindFlags = 0;

  ComPtr<ID3D11Texture2D> pCopyTexture = nullptr;
  if (SUCCEEDED(pDevice->CreateTexture2D(&desc, nullptr, &pCopyTexture)))
  {
    // take copy
    pImdContext->CopyResource(pCopyTexture.Get(), backbuffer->Get());

    D3D11_MAPPED_SUBRESOURCE res;
    if (SUCCEEDED(pImdContext->Map(pCopyTexture.Get(), 0, D3D11_MAP_READ, 0, &res)))
    {
      m_width = desc.Width;
      m_height = desc.Height;
      m_stride = res.RowPitch;
      m_buffer = new unsigned char[m_height * m_stride];
      if (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
      {
        // convert R10G10B10A2 -> B8G8R8A8
        for (int y = 0; y < m_height; y++)
        {
          uint32_t* pixels10 = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(res.pData) + y * res.RowPitch);
          uint8_t* pixels8 = m_buffer + y * m_stride;

          for (int x = 0; x < m_width; x++, pixels10++, pixels8 += 4)
          {
            // actual bit per channel is A2B10G10R10
            uint32_t pixel = *pixels10;
            // R
            pixels8[2] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // G
            pixel >>= 10;
            pixels8[1] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // B
            pixel >>= 10;
            pixels8[0] = static_cast<uint8_t>((pixel & 0x3FF) * 255 / 1023);
            // A
            pixels8[3] = 0xFF;
          }
        }
      }
      else
        memcpy(m_buffer, res.pData, m_height * m_stride);
      pImdContext->Unmap(pCopyTexture.Get(), 0);
    }
    else
      CLog::LogFunction(LOGERROR, __FUNCTION__, "MAP_READ failed.");
  }
#elif defined(HAS_GL) || defined(HAS_GLES)

  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  CServiceBroker::GetGUI()->GetWindowManager().Render();
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
#if HAS_GLES >= 2
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#else
  glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)surface);
#endif

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

#if defined(HAS_LIBAMCODEC)
  // Captures the current visible videobuffer and blend it into m_buffer (captured overlay)
  CScreenshotAML::CaptureVideoFrame(m_buffer, m_width, m_height);
#endif

#else
  //nothing to take a screenshot from
  return false;
#endif

  return true;
}

void CScreenShot::TakeScreenshot(const std::string &filename, bool sync)
{

  CScreenshotSurface surface;
  if (!surface.capture())
  {
    CLog::Log(LOGERROR, "Screenshot %s failed", CURL::GetRedacted(filename).c_str());
    return;
  }

  CLog::Log(LOGDEBUG, "Saving screenshot %s", CURL::GetRedacted(filename).c_str());

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
      CLog::Log(LOGERROR, "Unable to write screenshot %s", CURL::GetRedacted(filename).c_str());

    delete [] surface.m_buffer;
    surface.m_buffer = NULL;
  }
  else
  {
    //make sure the file exists to avoid concurrency issues
    XFILE::CFile file;
    if (file.OpenForWrite(filename))
      file.Close();
    else
      CLog::Log(LOGERROR, "Unable to create file %s", CURL::GetRedacted(filename).c_str());

    //write .png file asynchronous with CThumbnailWriter, prevents stalling of the render thread
    //buffer is deleted from CThumbnailWriter
    CThumbnailWriter* thumbnailwriter = new CThumbnailWriter(surface.m_buffer, surface.m_width, surface.m_height, surface.m_stride, filename);
    CJobManager::GetInstance().AddJob(thumbnailwriter, NULL);
    surface.m_buffer = NULL;
  }
}

void CScreenShot::TakeScreenshot()
{
  std::shared_ptr<CSettingPath> screenshotSetting = std::static_pointer_cast<CSettingPath>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_DEBUG_SCREENSHOTPATH));
  if (!screenshotSetting)
    return;

  std::string strDir = screenshotSetting->GetValue();
  if (strDir.empty())
  {
    if (!CGUIControlButtonSetting::GetPath(screenshotSetting, &g_localizeStrings))
      return;

    strDir = screenshotSetting->GetValue();
  }

  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.empty())
  {
    std::string file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot%03d.png"), 999);

    if (!file.empty())
    {
      TakeScreenshot(file, false);
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}
