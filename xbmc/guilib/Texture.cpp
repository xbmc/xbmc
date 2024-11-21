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
#include "guilib/TextureFormats.h"
#include "guilib/iimage.h"
#include "guilib/imagefactory.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
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
                                                 CAspectRatio::AspectRatio aspectRatio,
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
  if (texture->LoadFromFileInternal(texturePath, idealWidth, idealHeight, aspectRatio, strMimeType))
    return texture;
  return {};
}

std::unique_ptr<CTexture> CTexture::LoadFromFileInMemory(unsigned char* buffer,
                                                         size_t bufferSize,
                                                         const std::string& mimeType,
                                                         unsigned int idealWidth,
                                                         unsigned int idealHeight,
                                                         CAspectRatio::AspectRatio aspectRatio)
{
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture();
  if (texture->LoadFromFileInMem(buffer, bufferSize, mimeType, idealWidth, idealHeight,
                                 aspectRatio))
    return texture;
  return {};
}

bool CTexture::LoadFromFileInternal(const std::string& texturePath,
                                    unsigned int idealWidth,
                                    unsigned int idealHeight,
                                    CAspectRatio::AspectRatio aspectRatio,
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
    if (xbtFile.GetKDFormatType())
    {
      return UploadFromMemory(xbtFile.GetImageWidth(), xbtFile.GetImageHeight(), 0, buf.data(),
                              xbtFile.GetKDFormat(), xbtFile.GetKDAlpha(), xbtFile.GetKDSwizzle());
    }
    else if (xbtFile.GetImageFormat() == XB_FMT_A8R8G8B8)
    {
      KD_TEX_ALPHA alpha = xbtFile.HasImageAlpha() ? KD_TEX_ALPHA_STRAIGHT : KD_TEX_ALPHA_OPAQUE;
      return UploadFromMemory(xbtFile.GetImageWidth(), xbtFile.GetImageHeight(), 0, buf.data(),
                              KD_TEX_FMT_SDR_BGRA8, alpha, KD_TEX_SWIZ_RGBA);
    }
    else
    {
      return false;
    }
  }

  IImage* pImage;

  if(strMimeType.empty())
    pImage = ImageFactory::CreateLoader(texturePath);
  else
    pImage = ImageFactory::CreateLoaderFromMimeType(strMimeType);

  if (!LoadIImage(pImage, buf.data(), buf.size(), idealWidth, idealHeight, aspectRatio))
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
                                 unsigned int idealWidth,
                                 unsigned int idealHeight,
                                 CAspectRatio::AspectRatio aspectRatio)
{
  if (!buffer || !size)
    return false;

  IImage* pImage = ImageFactory::CreateLoaderFromMimeType(mimeType);
  if (!LoadIImage(pImage, buffer, size, idealWidth, idealHeight, aspectRatio))
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
                          unsigned int idealWidth,
                          unsigned int idealHeight,
                          CAspectRatio::AspectRatio aspectRatio)
{
  if (pImage == nullptr)
    return false;

  unsigned int maxTextureSize = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
  if (!pImage->LoadImageFromMemory(buffer, bufSize, maxTextureSize, maxTextureSize))
    return false;

  if (pImage->Width() == 0 || pImage->Height() == 0)
    return false;

  unsigned int width = idealWidth ? idealWidth : pImage->Width();
  unsigned int height = idealHeight ? idealHeight : pImage->Height();

  if (aspectRatio == CAspectRatio::STRETCH)
  {
    // noop
  }
  else if (aspectRatio == CAspectRatio::CENTER)
  {
    width = pImage->Width();
    height = pImage->Height();
  }
  else
  {
    float aspect = (float)(pImage->Width()) / pImage->Height();
    unsigned int heightFromWidth = (unsigned int)(width / aspect + 0.5f);
    if (aspectRatio == CAspectRatio::SCALE)
    {
      if (heightFromWidth > height)
        height = heightFromWidth;
      else
        width = (unsigned int)(height * aspect + 0.5f);
    }
    else if (aspectRatio == CAspectRatio::KEEP)
    {
      if (heightFromWidth > height)
        width = (unsigned int)(height * aspect + 0.5f);
      else
        height = heightFromWidth;
    }
  }

  if (width > maxTextureSize || height > maxTextureSize)
  {
    float aspect = (float)width / height;
    if (width >= height)
    {
      width = maxTextureSize;
      height = (unsigned int)(width / aspect + 0.5f);
    }
    else
    {
      height = maxTextureSize;
      width = (unsigned int)(height * aspect + 0.5f);
    }
  }

  // align all textures so that they have an even width
  // in some circumstances when we downsize a thumbnail
  // which has an uneven number of pixels in width
  // we crash in CPicture::ScaleImage in ffmpegs swscale
  // because it tries to access beyond the source memory
  // (happens on osx and ios)
  // UPDATE: don't just update to be on an even width;
  // ffmpegs swscale relies on a 16-byte stride on some systems
  // so the textureWidth needs to be a multiple of 16. see ffmpeg
  // swscale headers for more info.
  unsigned int textureWidth = ((width + 15) / 16) * 16;

  Allocate(textureWidth, height, XB_FMT_A8R8G8B8);

  m_imageWidth = std::min(m_imageWidth, textureWidth);

  if (m_pixels == nullptr)
    return false;

  if (!pImage->Decode(m_pixels, width, height, GetPitch(), XB_FMT_A8R8G8B8))
    return false;

  if (pImage->Orientation())
    m_orientation = pImage->Orientation() - 1;

  m_textureAlpha = pImage->hasAlpha() ? KD_TEX_ALPHA_STRAIGHT : KD_TEX_ALPHA_OPAQUE;
  m_originalWidth = pImage->originalWidth();
  m_originalHeight = pImage->originalHeight();
  m_imageWidth = pImage->Width();
  m_imageHeight = pImage->Height();

  ClampToEdge();

  return true;
}

void CTexture::LoadToGPUAsync()
{
  // Already in main context?
  if (CServiceBroker::GetWinSystem()->HasContext())
    return;

  if (!CServiceBroker::GetWinSystem()->BindTextureUploadContext())
    return;

  LoadToGPU();

  SyncGPU();

  CServiceBroker::GetWinSystem()->UnbindTextureUploadContext();
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
  m_textureAlpha = hasAlpha ? KD_TEX_ALPHA_STRAIGHT : KD_TEX_ALPHA_OPAQUE;
  Update(width, height, pitch, format, pixels, false);
  return true;
}

bool CTexture::UploadFromMemory(unsigned int width,
                                unsigned int height,
                                unsigned int pitch,
                                unsigned char* pixels,
                                KD_TEX_FMT format,
                                KD_TEX_ALPHA alpha,
                                KD_TEX_SWIZ swizzle)
{
  m_imageWidth = m_textureWidth = m_originalWidth = width;
  m_imageHeight = m_textureHeight = m_originalHeight = height;
  m_textureFormat = format;
  m_textureAlpha = alpha;
  m_textureSwizzle = swizzle;

  if (!SupportsFormat(m_textureFormat, m_textureSwizzle) && !ConvertToLegacy(width, height, pixels))
  {
    CLog::LogF(
        LOGERROR,
        "Failed to upload texture. Format {} and swizzle {} not supported by the texture pipeline.",
        m_textureFormat, m_textureSwizzle);

    m_loadedToGPU = true;
    return false;
  }

  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    if (m_pixels)
    {
      LoadToGPU();
    }
    else
    {
      // just a borrowed buffer
      m_pixels = pixels;
      m_bCacheMemory = true;
      LoadToGPU();
      m_bCacheMemory = false;
      m_pixels = nullptr;
    }
  }
  else if (!m_pixels)
  {
    size_t size = GetPitch() * GetRows();
    m_pixels = static_cast<unsigned char*>(KODI::MEMORY::AlignedMalloc(size, 32));
    if (m_pixels == nullptr)
    {
      CLog::LogF(LOGERROR, "Could not allocate {} bytes. Out of memory.", size);
      return false;
    }
    std::memcpy(m_pixels, pixels, size);
  }

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
