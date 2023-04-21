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
#include "guilib/TextureFormats.h"
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
CTexture::CTexture(unsigned int width, unsigned int height, unsigned int format)
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

void CTexture::Allocate(unsigned int width, unsigned int height, unsigned int format, bool scalable)
{
  m_imageWidth = m_originalWidth = width;
  m_imageHeight = m_originalHeight = height;
  m_format = format;
  m_orientation = 0;

  m_textureWidth = m_imageWidth;
  m_textureHeight = m_imageHeight;

  if (m_format & XB_FMT_DXT_MASK)
  {
    while (GetPitch() < CServiceBroker::GetRenderSystem()->GetMinDXTPitch())
      m_textureWidth += GetBlockSize();
  }

  if (!CServiceBroker::GetRenderSystem()->SupportsNPOT((m_format & XB_FMT_DXT_MASK) != 0))
  {
    m_textureWidth = PadPow2(m_textureWidth);
    m_textureHeight = PadPow2(m_textureHeight);
  }

  if (m_format & XB_FMT_DXT_MASK)
  {
    // DXT textures must be a multiple of 4 in width and height
    m_textureWidth = ((m_textureWidth + 3) / 4) * 4;
    m_textureHeight = ((m_textureHeight + 3) / 4) * 4;
  }
  else if (scalable)
  {
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
    m_textureWidth = ((m_textureWidth + 15) / 16) * 16;
  }

  // check for max texture size
  #define CLAMP(x, y) { if (x > y) x = y; }
  CLAMP(m_textureWidth, CServiceBroker::GetRenderSystem()->GetMaxTextureSize());
  CLAMP(m_textureHeight, CServiceBroker::GetRenderSystem()->GetMaxTextureSize());
  CLAMP(m_imageWidth, m_textureWidth);
  CLAMP(m_imageHeight, m_textureHeight);

  KODI::MEMORY::AlignedFree(m_pixels);
  m_pixels = NULL;
  if (GetPitch(m_textureWidth, m_format) * GetRows(m_textureHeight, m_format) > 0)
  {
    size_t size = GetPitch(m_textureWidth, m_format) * GetRows(m_textureHeight, m_format);
    m_pixels = static_cast<unsigned char*>(KODI::MEMORY::AlignedMalloc(size, 32));

    if (m_pixels == nullptr)
    {
      CLog::Log(LOGERROR, "{} - Could not allocate {} bytes. Out of memory.", __FUNCTION__, size);
    }
  }
}

void CTexture::Update(unsigned int width,
                      unsigned int height,
                      unsigned int pitch,
                      unsigned int format,
                      const unsigned char* pixels,
                      bool loadToGPU)
{
  if (pixels == NULL)
    return;

  if (format & XB_FMT_DXT_MASK)
    return;

  if (IsGPUFormatSupported(format))
    Allocate(width, height, format, false);
  else
    Allocate(width, height, XB_FMT_A8R8G8B8, false);

  if (m_pixels == nullptr)
    return;

  unsigned int srcPitch = pitch ? pitch : GetPitch(width, format);
  unsigned int srcRows = GetRows(height, format);
  unsigned int dstPitch = GetPitch(m_textureWidth, format);
  unsigned int dstRows = GetRows(m_textureHeight, format);

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

  if (!IsGPUFormatSupported(format))
    ConvertToBGRA(format);

  ClampToEdge();

  if (loadToGPU)
    LoadToGPU();
}

void CTexture::ClampToEdge()
{
  if (m_pixels == nullptr)
    return;

  unsigned int imagePitch = GetPitch(m_imageWidth);
  unsigned int imageRows = GetRows(m_imageHeight);
  unsigned int texturePitch = GetPitch(m_textureWidth);
  unsigned int textureRows = GetRows(m_textureHeight);
  if (imagePitch < texturePitch)
  {
    unsigned int blockSize = GetBlockSize();
    unsigned char *src = m_pixels + imagePitch - blockSize;
    unsigned char *dst = m_pixels;
    for (unsigned int y = 0; y < imageRows; y++)
    {
      for (unsigned int x = imagePitch; x < texturePitch; x += blockSize)
        memcpy(dst + x, src, blockSize);
      dst += texturePitch;
    }
  }

  if (imageRows < textureRows)
  {
    unsigned char *dst = m_pixels + imageRows * texturePitch;
    for (unsigned int y = imageRows; y < textureRows; y++)
    {
      memcpy(dst, dst - texturePitch, texturePitch);
      dst += texturePitch;
    }
  }
}

std::unique_ptr<CTexture> CTexture::LoadFromFile(const std::string& texturePath,
                                                 unsigned int idealWidth,
                                                 unsigned int idealHeight,
                                                 bool requirePixels,
                                                 const std::string& strMimeType,
                                                 unsigned int format)
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
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture(0, 0, format);
  if (texture->LoadFromFileInternal(texturePath, idealWidth, idealHeight, requirePixels, strMimeType))
    return texture;
  return {};
}

std::unique_ptr<CTexture> CTexture::LoadFromFileInMemory(unsigned char* buffer,
                                                         size_t bufferSize,
                                                         const std::string& mimeType,
                                                         unsigned int idealWidth,
                                                         unsigned int idealHeight,
                                                         unsigned int format)
{
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture(0, 0, format);
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
      // if we don't request a specific format, the decoder can suggest a compatible one.
      if (m_format == XB_FMT_UNKNOWN)
        m_format = pImage->GetCompatibleFormat();
      // if the decoder can't write to the texture, fall back to our standard four channel texture
      else if (!pImage->IsFormatSupported(m_format))
        m_format = XB_FMT_A8R8G8B8;
      // if not supported on the GPU, we fall back
      if (!IsGPUFormatSupported(m_format))
        m_format = XB_FMT_A8R8G8B8;
      Allocate(pImage->Width(), pImage->Height(), m_format);
      if (m_pixels != nullptr &&
          pImage->Decode(m_pixels, GetTextureWidth(), GetRows(), GetPitch(), m_format))
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
                              unsigned int format,
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
                            unsigned int format,
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

unsigned int CTexture::PadPow2(unsigned int x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

bool CTexture::SwapBlueRed(unsigned char* pixels,
                           unsigned int height,
                           unsigned int pitch,
                           unsigned int elements,
                           unsigned int offset)
{
  if (!pixels) return false;
  unsigned char *dst = pixels;
  for (unsigned int y = 0; y < height; y++)
  {
    dst = pixels + (y * pitch);
    for (unsigned int x = 0; x < pitch; x+=elements)
      std::swap(dst[x+offset], dst[x+2+offset]);
  }
  return true;
}

unsigned int CTexture::GetPitch(unsigned int width) const
{
  return GetPitch(width, m_format);
}

unsigned int CTexture::GetPitch(unsigned int width, unsigned int format) const
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return ((width + 3) / 4) * 16;
  case XB_FMT_R8:
  case XB_FMT_A8:
  case XB_FMT_L8:
    return width;
  case XB_FMT_L8A8:
    return width * 2;
  case XB_FMT_RGB8:
    return (((width + 1)* 3 / 4) * 4);
  case XB_FMT_RGBA8:
  case XB_FMT_A8R8G8B8:
  default:
    return width*4;
  }
}

unsigned int CTexture::GetRows(unsigned int height) const
{
  return GetRows(height, m_format);
}

unsigned int CTexture::GetRows(unsigned int height, unsigned int format) const
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return (height + 3) / 4;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return (height + 3) / 4;
  default:
    return height;
  }
}

unsigned int CTexture::GetBlockSize() const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return 16;
  case XB_FMT_A8:
  case XB_FMT_L8:
  case XB_FMT_R8:
    return 1;
  case XB_FMT_L8A8:
    return 2;
  default:
    return 4;
  }
}

bool CTexture::HasAlpha() const
{
  return m_hasAlpha;
}

bool CTexture::IsAlphaTexture() const
{
  return m_format == XB_FMT_A8;
}

void CTexture::SetMipmapping()
{
  m_mipmapping = true;
}

bool CTexture::IsMipmapped() const
{
  return m_mipmapping;
}

void CTexture::ConvertToBGRA(uint32_t format)
{
  size_t size = GetPitch(m_textureWidth, format) * GetRows(m_textureHeight, format);

  if (format == XB_FMT_A8)
  {
    for (int32_t i = size - 1; i >= 0; i--)
    {
      m_pixels[i * 4 + 3] = m_pixels[i];
      m_pixels[i * 4 + 2] = char(0xff);
      m_pixels[i * 4 + 1] = char(0xff);
      m_pixels[i * 4] = char(0xff);
    }
  }
  else if (format == XB_FMT_L8)
  {
    for (int32_t i = size - 1; i >= 0; i--)
    {
      m_pixels[i * 4 + 3] = char(0xff);
      m_pixels[i * 4 + 2] = m_pixels[i];
      m_pixels[i * 4 + 1] = m_pixels[i];
      m_pixels[i * 4] = m_pixels[i];
    }
  }
  else if (format == XB_FMT_L8A8)
  {
    for (int32_t i = size / 2 - 1; i >= 0; i--)
    {
      m_pixels[i * 4 + 3] = m_pixels[i * 2 + 1];
      m_pixels[i * 4 + 2] = m_pixels[i * 2];
      m_pixels[i * 4 + 1] = m_pixels[i * 2];
      m_pixels[i * 4] = m_pixels[i * 2];
    }
  }

  m_format = XB_FMT_A8R8G8B8;
}
