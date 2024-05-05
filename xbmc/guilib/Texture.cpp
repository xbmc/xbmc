/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Texture.h"

#include "DDSImage.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "commons/ilog.h"
#include "filesystem/File.h"
#include "filesystem/ResourceFile.h"
#include "filesystem/XbtFile.h"
#include "guilib/TextureBase.h"
#include "guilib/iimage.h"
#include "guilib/imagefactory.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#if defined(TARGET_DARWIN_EMBEDDED)
#include <ImageIO/ImageIO.h>
#include "filesystem/File.h"
#endif
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/AndroidAppFile.h"
#endif
#include "rendering/RenderSystem.h"
#include "utils/MemUtils.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>

/************************************************************************/
/*                                                                      */
/************************************************************************/
CTexture::CTexture(unsigned int width, unsigned int height, XB_FMT format)
{
  m_pixels = NULL;
  m_loadedToGPU = false;
  Allocate(width, height, format);
}

CTexture::~CTexture()
{
  KODI::MEMORY::AlignedFree(m_pixels);
  m_pixels = NULL;
}

void CTexture::Update(unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      XB_FMT format,
                      const unsigned char* pixels,
                      bool loadToGPU)
{
  if (pixels == NULL)
    return;

  if (format & XB_FMT_DXT_MASK)
    return;

  Allocate(width, height, format);

  if (m_pixels == nullptr)
    return;

  unsigned int srcPitch = pitch ? pitch : GetPitch(width);
  unsigned int srcRows = GetRows(height);
  unsigned int dstPitch = GetPitch(m_textureWidth);
  unsigned int dstRows = GetRows(m_textureHeight);

  if (srcPitch == dstPitch)
    memcpy(m_pixels, pixels, srcPitch * std::min(srcRows, dstRows));
  else
  {
    const unsigned char *src = pixels;
    unsigned char* dst = m_pixels;
    for (unsigned int y = 0; y < srcRows && y < dstRows; y++)
    {
      memcpy(dst, src, std::min(srcPitch, dstPitch));
      src += srcPitch;
      dst += dstPitch;
    }
  }
  ClampToEdge();

  if (loadToGPU)
    LoadToGPU();
}

std::unique_ptr<CTexture> CTexture::LoadFromFile(const std::string& texturePath,
                                                 unsigned int idealWidth,
                                                 unsigned int idealHeight,
                                                 bool requirePixels,
                                                 const std::string& strMimeType)
{
#if defined(TARGET_ANDROID)
  CURL url(texturePath);
  if (url.IsProtocol("androidapp"))
  {
    XFILE::CFileAndroidApp file;
    if (file.Open(url))
    {
      unsigned char* inputBuff;
      unsigned int width;
      unsigned int height;
      unsigned int inputBuffSize = file.ReadIcon(&inputBuff, &width, &height);
      file.Close();
      if (!inputBuffSize)
        return NULL;

      std::unique_ptr<CTexture> texture = CTexture::CreateTexture();
      texture->LoadFromMemory(width, height, width*4, XB_FMT_RGBA8, true, inputBuff);
      delete[] inputBuff;
      return texture;
    }
  }
#endif
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture();
  if (texture->LoadFromFileInternal(texturePath, idealWidth, idealHeight, requirePixels, strMimeType))
    return texture;
  return {};
}

std::unique_ptr<CTexture> CTexture::LoadFromFileInMemory(unsigned char* buffer,
                                                         size_t bufferSize,
                                                         const std::string& mimeType,
                                                         unsigned int idealWidth,
                                                         unsigned int idealHeight)
{
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture();
  if (texture->LoadFromFileInMem(buffer, bufferSize, mimeType, idealWidth, idealHeight))
    return texture;
  return {};
}

bool CTexture::LoadFromFileInternal(const std::string& texturePath,
                                    unsigned int maxWidth,
                                    unsigned int maxHeight,
                                    bool requirePixels,
                                    const std::string& strMimeType)
{
  if (URIUtils::HasExtension(texturePath, ".dds"))
  { // special case for DDS images
    CDDSImage image;
    if (image.ReadFile(texturePath))
    {
      Update(image.GetWidth(), image.GetHeight(), 0, image.GetFormat(), image.GetData(), false);
      return true;
    }
    return false;
  }

  unsigned int width = maxWidth ? std::min(maxWidth, CServiceBroker::GetRenderSystem()->GetMaxTextureSize()) :
                                  CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
  unsigned int height = maxHeight ? std::min(maxHeight, CServiceBroker::GetRenderSystem()->GetMaxTextureSize()) :
                                    CServiceBroker::GetRenderSystem()->GetMaxTextureSize();

  // Read image into memory to use our vfs
  XFILE::CFile file;
  std::vector<uint8_t> buf;

  if (file.LoadFile(texturePath, buf) <= 0)
    return false;

  CURL url(texturePath);
  // make sure resource:// paths are properly resolved
  if (url.IsProtocol("resource"))
  {
    std::string translatedPath;
    if (XFILE::CResourceFile::TranslatePath(url, translatedPath))
      url.Parse(translatedPath);
  }

  // handle xbt:// paths differently because it allows loading the texture directly from memory
  if (url.IsProtocol("xbt"))
  {
    XFILE::CXbtFile xbtFile;
    if (!xbtFile.Open(url))
      return false;

    return LoadFromMemory(xbtFile.GetImageWidth(), xbtFile.GetImageHeight(), 0,
                          xbtFile.GetImageFormat(), xbtFile.HasImageAlpha(), buf.data());
  }

  IImage* pImage;

  if(strMimeType.empty())
    pImage = ImageFactory::CreateLoader(texturePath);
  else
    pImage = ImageFactory::CreateLoaderFromMimeType(strMimeType);

  if (!LoadIImage(pImage, buf.data(), buf.size(), width, height))
  {
    CLog::Log(LOGDEBUG, "{} - Load of {} failed.", __FUNCTION__, CURL::GetRedacted(texturePath));
    delete pImage;
    return false;
  }
  delete pImage;

  return true;
}

bool CTexture::LoadFromFileInMem(unsigned char* buffer,
                                 size_t size,
                                 const std::string& mimeType,
                                 unsigned int maxWidth,
                                 unsigned int maxHeight)
{
  if (!buffer || !size)
    return false;

  unsigned int width = maxWidth ? std::min(maxWidth, CServiceBroker::GetRenderSystem()->GetMaxTextureSize()) :
                                  CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
  unsigned int height = maxHeight ? std::min(maxHeight, CServiceBroker::GetRenderSystem()->GetMaxTextureSize()) :
                                    CServiceBroker::GetRenderSystem()->GetMaxTextureSize();

  IImage* pImage = ImageFactory::CreateLoaderFromMimeType(mimeType);
  if(!LoadIImage(pImage, buffer, size, width, height))
  {
    delete pImage;
    return false;
  }
  delete pImage;
  return true;
}

bool CTexture::LoadIImage(IImage* pImage,
                          unsigned char* buffer,
                          unsigned int bufSize,
                          unsigned int width,
                          unsigned int height)
{
  if(pImage != NULL && pImage->LoadImageFromMemory(buffer, bufSize, width, height))
  {
    if (pImage->Width() > 0 && pImage->Height() > 0)
    {
      Allocate(pImage->Width(), pImage->Height(), XB_FMT_A8R8G8B8);
      if (m_pixels != nullptr && pImage->Decode(m_pixels, GetTextureWidth(), GetRows(), GetPitch(), XB_FMT_A8R8G8B8))
      {
        if (pImage->Orientation())
          m_orientation = pImage->Orientation() - 1;
        m_hasAlpha = pImage->hasAlpha();
        m_originalWidth = pImage->originalWidth();
        m_originalHeight = pImage->originalHeight();
        m_imageWidth = pImage->Width();
        m_imageHeight = pImage->Height();
        ClampToEdge();
        return true;
      }
    }
  }
  return false;
}

bool CTexture::LoadFromMemory(unsigned int width,
                              unsigned int height,
                              unsigned int pitch,
                              XB_FMT format,
                              bool hasAlpha,
                              const unsigned char* pixels)
{
  m_imageWidth = m_originalWidth = width;
  m_imageHeight = m_originalHeight = height;
  m_format = format;
  m_hasAlpha = hasAlpha;
  Update(width, height, pitch, format, pixels, false);
  return true;
}

bool CTexture::LoadPaletted(unsigned int width,
                            unsigned int height,
                            unsigned int pitch,
                            XB_FMT format,
                            const unsigned char* pixels,
                            const COLOR* palette)
{
  if (pixels == NULL || palette == NULL)
    return false;

  Allocate(width, height, format);

  for (unsigned int y = 0; y < m_imageHeight; y++)
  {
    unsigned char *dest = m_pixels + y * GetPitch();
    const unsigned char *src = pixels + y * pitch;
    for (unsigned int x = 0; x < m_imageWidth; x++)
    {
      COLOR col = palette[*src++];
      *dest++ = col.b;
      *dest++ = col.g;
      *dest++ = col.r;
      *dest++ = col.x;
    }
  }
  ClampToEdge();
  return true;
}
