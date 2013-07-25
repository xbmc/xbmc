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

#include "Texture.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "DDSImage.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#if defined(TARGET_DARWIN_IOS)
#include <ImageIO/ImageIO.h>
#include "filesystem/File.h"
#include "osx/DarwinUtils.h"
#endif
#if defined(TARGET_ANDROID)
#include "URL.h"
#include "filesystem/AndroidAppFile.h"
#endif

#if defined(HAS_OMXPLAYER)
#include "xbmc/cores/omxplayer/OMXImage.h"
#endif

/************************************************************************/
/*                                                                      */
/************************************************************************/
CBaseTexture::CBaseTexture(unsigned int width, unsigned int height, unsigned int format)
 : m_hasAlpha( true )
{
  m_pixels = NULL;
  m_loadedToGPU = false;
  Allocate(width, height, format);
}

CBaseTexture::~CBaseTexture()
{
  delete[] m_pixels;
}

void CBaseTexture::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
  m_imageWidth = m_originalWidth = width;
  m_imageHeight = m_originalHeight = height;
  m_format = format;
  m_orientation = 0;

  m_textureWidth = m_imageWidth;
  m_textureHeight = m_imageHeight;

  if (m_format & XB_FMT_DXT_MASK)
    while (GetPitch() < g_Windowing.GetMinDXTPitch())
      m_textureWidth += GetBlockSize();

  if (!g_Windowing.SupportsNPOT((m_format & XB_FMT_DXT_MASK) != 0))
  {
    m_textureWidth = PadPow2(m_textureWidth);
    m_textureHeight = PadPow2(m_textureHeight);
  }
  if (m_format & XB_FMT_DXT_MASK)
  { // DXT textures must be a multiple of 4 in width and height
    m_textureWidth = ((m_textureWidth + 3) / 4) * 4;
    m_textureHeight = ((m_textureHeight + 3) / 4) * 4;
  }
  else
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
  CLAMP(m_textureWidth, g_Windowing.GetMaxTextureSize());
  CLAMP(m_textureHeight, g_Windowing.GetMaxTextureSize());
  CLAMP(m_imageWidth, m_textureWidth);
  CLAMP(m_imageHeight, m_textureHeight);
  delete[] m_pixels;
  m_pixels = new unsigned char[GetPitch() * GetRows()];
}

void CBaseTexture::Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU)
{
  if (pixels == NULL)
    return;

  if (format & XB_FMT_DXT_MASK && !g_Windowing.SupportsDXT())
  { // compressed format that we don't support
    Allocate(width, height, XB_FMT_A8R8G8B8);
    CDDSImage::Decompress(m_pixels, std::min(width, m_textureWidth), std::min(height, m_textureHeight), GetPitch(m_textureWidth), pixels, format);
  }
  else
  {
    Allocate(width, height, format);

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
  }
  ClampToEdge();

  if (loadToGPU)
    LoadToGPU();
}

void CBaseTexture::ClampToEdge()
{
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

CBaseTexture *CBaseTexture::LoadFromFile(const CStdString& texturePath, unsigned int idealWidth, unsigned int idealHeight, bool autoRotate)
{
#if defined(TARGET_ANDROID)
  CURL url(texturePath);
  if (url.GetProtocol() == "androidapp")
  {
    XFILE::CFileAndroidApp file;
    if (file.Open(url))
    {
      unsigned int imgsize = (unsigned int)file.GetLength();
      unsigned char* inputBuff = new unsigned char[imgsize];
      unsigned int inputBuffSize = file.Read(inputBuff, imgsize);
      file.Close();
      if (inputBuffSize != imgsize)
      {
        delete [] inputBuff;
        return NULL;
      }
      CTexture *texture = new CTexture();
      unsigned int width = file.GetIconWidth();
      unsigned int height = file.GetIconHeight();
      texture->LoadFromMemory(width, height, width*4, XB_FMT_RGBA8, true, inputBuff);
      delete [] inputBuff;
      return texture;
    }
  }
#endif
  CTexture *texture = new CTexture();
  if (texture->LoadFromFileInternal(texturePath, idealWidth, idealHeight, autoRotate))
    return texture;
  delete texture;
  return NULL;
}

CBaseTexture *CBaseTexture::LoadFromFileInMemory(unsigned char *buffer, size_t bufferSize, const std::string &mimeType, unsigned int idealWidth, unsigned int idealHeight)
{
  CTexture *texture = new CTexture();
  if (texture->LoadFromFileInMem(buffer, bufferSize, mimeType, idealWidth, idealHeight))
    return texture;
  delete texture;
  return NULL;
}

bool CBaseTexture::LoadFromFileInternal(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool autoRotate)
{
#if defined(HAS_OMXPLAYER)
  if (URIUtils::HasExtension(texturePath, ".jpg|.tbn")
      /*|| URIUtils::HasExtension(texturePath, ".png")*/)
  {
    COMXImage omx_image;

    if(omx_image.ReadFile(texturePath))
    {
      if(omx_image.Decode(maxWidth, maxHeight))
      {
        Allocate(omx_image.GetDecodedWidth(), omx_image.GetDecodedHeight(), XB_FMT_A8R8G8B8);

        if(!m_pixels)
        {
          CLog::Log(LOGERROR, "Texture manager (OMX) out of memory");
          omx_image.Close();
          return false;
        }

        m_originalWidth  = omx_image.GetOriginalWidth();
        m_originalHeight = omx_image.GetOriginalHeight();

        m_hasAlpha = omx_image.IsAlpha();

        if (autoRotate && omx_image.GetOrientation())
          m_orientation = omx_image.GetOrientation() - 1;

        if(m_textureWidth != omx_image.GetDecodedWidth() || m_textureHeight != omx_image.GetDecodedHeight())
        {
          unsigned int imagePitch = GetPitch(m_imageWidth);
          unsigned int imageRows = GetRows(m_imageHeight);
          unsigned int texturePitch = GetPitch(m_textureWidth);

          unsigned char *src = omx_image.GetDecodedData();
          unsigned char *dst = m_pixels;
          for (unsigned int y = 0; y < imageRows; y++)
          {
            memcpy(dst, src, imagePitch);
            src += imagePitch;
            dst += texturePitch;
          }
        }
        else
        {
          if(omx_image.GetDecodedData())
          {
            int size = ( ( GetPitch() * GetRows() ) > omx_image.GetDecodedSize() ) ?
                             omx_image.GetDecodedSize() : ( GetPitch() * GetRows() );

            memcpy(m_pixels, (unsigned char *)omx_image.GetDecodedData(), size);
          }
        }

        omx_image.Close();

        return true;
      }
      else
      {
        omx_image.Close();
      }
    }
    // this limits the sizes of jpegs we failed to decode
    omx_image.ClampLimits(maxWidth, maxHeight);
  }
#endif
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

  unsigned int width = maxWidth ? std::min(maxWidth, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();
  unsigned int height = maxHeight ? std::min(maxHeight, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();

  // Read image into memory to use our vfs
  unsigned char *inputBuff = NULL;
  unsigned int inputBuffSize = 0;

  XFILE::CFile file;
  if (file.Open(texturePath.c_str(), READ_TRUNCATED))
  {
    /*
     GetLength() will typically return values that fall into three cases:
       1. The real filesize. This is the typical case.
       2. Zero. This is the case for some http:// streams for example.
       3. Some value smaller than the real filesize. This is the case for an expanding file.

     In order to handle all three cases, we read the file in chunks, relying on Read()
     returning 0 at EOF.  To minimize (re)allocation of the buffer, the chunksize in
     cases 1 and 3 is set to one byte larger** than the value returned by GetLength().
     The chunksize in case 2 is set to the larger of 64k and GetChunkSize().

     We fill the buffer entirely before reallocation.  Thus, reallocation never occurs in case 1
     as the buffer is larger than the file, so we hit EOF before we hit the end of buffer.

     To minimize reallocation, we double the chunksize each time up to a maxchunksize of 2MB.
     */
    unsigned int filesize = (unsigned int)file.GetLength();
    unsigned int chunksize = filesize ? (filesize + 1) : std::max(65536U, (unsigned int)file.GetChunkSize());
    unsigned int maxchunksize = 2048*1024U; /* max 2MB chunksize */
    unsigned char *tempinputBuff = NULL;

    unsigned int total_read = 0, free_space = 0;
    while (true)
    {
      if (!free_space)
      { // (re)alloc
        inputBuffSize += chunksize;
        tempinputBuff = (unsigned char *)realloc(inputBuff, inputBuffSize);
        if (!tempinputBuff)
        {
          CLog::Log(LOGERROR, "%s unable to allocate buffer of size %u", __FUNCTION__, inputBuffSize);
          free(inputBuff);
          file.Close();
          return false;
        }
        inputBuff = tempinputBuff;
        free_space = chunksize;
        chunksize = std::min(chunksize*2, maxchunksize);
      }
      unsigned int read = file.Read(inputBuff + total_read, free_space);
      free_space -= read;
      total_read += read;
      if (!read)
        break;
    }
    inputBuffSize = total_read;
    file.Close();

    if (inputBuffSize == 0)
      return false;
  }
  else
    return false;

  CURL url(texturePath);
  IImage* pImage = ImageFactory::CreateLoader(url);
  if(!LoadIImage(pImage, inputBuff, inputBuffSize, width, height, autoRotate))
  {
    delete pImage;
    pImage = NULL;
    pImage = ImageFactory::CreateFallbackLoader(texturePath);
    if(!LoadIImage(pImage, inputBuff, inputBuffSize, width, height))
    {
      CLog::Log(LOGDEBUG, "%s - Load of %s failed.", __FUNCTION__, texturePath.c_str());
      delete pImage;
      free(inputBuff);
      return false;
    }
  }
  delete pImage;
  free(inputBuff);

  return true;
}

bool CBaseTexture::LoadFromFileInMem(unsigned char* buffer, size_t size, const std::string& mimeType, unsigned int maxWidth, unsigned int maxHeight)
{
  if (!buffer || !size)
    return false;

  unsigned int width = maxWidth ? std::min(maxWidth, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();
  unsigned int height = maxHeight ? std::min(maxHeight, g_Windowing.GetMaxTextureSize()) : g_Windowing.GetMaxTextureSize();

  IImage* pImage = ImageFactory::CreateLoaderFromMimeType(mimeType);
  if(!LoadIImage(pImage, buffer, size, width, height))
  {
    delete pImage;
    pImage = NULL;
    pImage = ImageFactory::CreateFallbackLoader(mimeType);
    if(!LoadIImage(pImage, buffer, size, width, height))
    {
      delete pImage;
      return false;
    }
  }
  delete pImage;
  return true;
}

bool CBaseTexture::LoadIImage(IImage *pImage, unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height, bool autoRotate)
{
  if(pImage != NULL && pImage->LoadImageFromMemory(buffer, bufSize, width, height))
  {
    if (pImage->Width() > 0 && pImage->Height() > 0)
    {
      Allocate(pImage->Width(), pImage->Height(), XB_FMT_A8R8G8B8);
      if (pImage->Decode(m_pixels, GetPitch(), XB_FMT_A8R8G8B8))
      {
        if (autoRotate && pImage->Orientation())
          m_orientation = pImage->Orientation() - 1;
        m_hasAlpha = pImage->hasAlpha();
        m_originalWidth = pImage->originalWidth();
        m_originalHeight = pImage->originalHeight();
        ClampToEdge();
        return true;
      }
    }
  }
  return false;
}

bool CBaseTexture::LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, bool hasAlpha, unsigned char* pixels)
{
  m_imageWidth = m_originalWidth = width;
  m_imageHeight = m_originalHeight = height;
  m_format = format;
  m_hasAlpha = hasAlpha;
  Update(width, height, pitch, format, pixels, false);
  return true;
}

bool CBaseTexture::LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette)
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

unsigned int CBaseTexture::PadPow2(unsigned int x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

bool CBaseTexture::SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, unsigned int elements, unsigned int offset)
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

unsigned int CBaseTexture::GetPitch(unsigned int width) const
{
  switch (m_format)
  {
  case XB_FMT_DXT1:
    return ((width + 3) / 4) * 8;
  case XB_FMT_DXT3:
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    return ((width + 3) / 4) * 16;
  case XB_FMT_A8:
    return width;
  case XB_FMT_RGB8:
    return (((width + 1)* 3 / 4) * 4);
  case XB_FMT_RGBA8:
  case XB_FMT_A8R8G8B8:
  default:
    return width*4;
  }
}

unsigned int CBaseTexture::GetRows(unsigned int height) const
{
  switch (m_format)
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

unsigned int CBaseTexture::GetBlockSize() const
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
    return 1;
  default:
    return 4;
  }
}

bool CBaseTexture::HasAlpha() const
{
  return m_hasAlpha;
}
