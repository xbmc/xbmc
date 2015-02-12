/*
 *      Copyright (C) 2014 Team Kodi
 *      http://kodi.tv
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

#include "GifHelper.h"
#include <algorithm>
#include <stdlib.h>
#include <cstring>

#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))
#define GIF_MAX_MEMORY 82944000U // about 79 MB, which is equivalent to 10 full hd frames.

class Gifreader
{
public:
  unsigned char* buffer;
  unsigned int buffSize;
  unsigned int readPosition;

  Gifreader() : buffer(NULL), buffSize(0), readPosition(0) {}
};

int ReadFromMemory(GifFileType* gif, GifByteType* gifbyte, int len)
{
  unsigned int alreadyRead = ((Gifreader*)gif->UserData)->readPosition;
  unsigned int buffSizeLeft = ((Gifreader*)gif->UserData)->buffSize - alreadyRead;
  int readBytes = len;

  if (len <= 0)
    readBytes = 0;

  if ((unsigned int)len > buffSizeLeft)
    readBytes = buffSizeLeft;

  if (readBytes > 0)
  {
    unsigned char* src = ((Gifreader*)gif->UserData)->buffer + alreadyRead;
    memcpy(gifbyte, src, readBytes);
    ((Gifreader*)gif->UserData)->readPosition += readBytes;
  }
  return readBytes;
}

int ReadFromVfs(GifFileType* gif, GifByteType* gifbyte, int len)
{
  CFile *gifFile = (CFile *)gif->UserData;
  return (int)gifFile->Read(gifbyte, len);
}


GifHelper::GifHelper() :
  m_imageSize(0),
  m_pitch(0),
  m_loops(0),
  m_numFrames(0),
  m_filename(""),
  m_gif(NULL),
  m_hasBackground(false),
  m_pTemplate(NULL),
  m_isAnimated(-1)
{
  m_backColor = new COLOR();
  memset(m_backColor, 0, sizeof(COLOR));
  m_frames.clear();
}

GifHelper::~GifHelper()
{
  int err;
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
  DGifCloseFile(m_gif, &err);
#else
  err = DGifCloseFile(m_gif);
#endif
  if (err == D_GIF_ERR_CLOSE_FAILED)
  {
    fprintf(stderr, "Gif::~Gif(): D_GIF_ERR_CLOSE_FAILED\n");
    free(m_gif);
  }
  Release();
  delete m_backColor;
}

void GifHelper::Release()
{
  delete[] m_pTemplate;
  m_pTemplate = NULL;
  m_globalPalette.clear();
  m_frames.clear();
}

void GifHelper::ConvertColorTable(std::vector<COLOR> &dest, ColorMapObject* src, unsigned int size)
{
  for (unsigned int i = 0; i < size; ++i)
  {
    COLOR c;

    c.r = src->Colors[i].Red;
    c.g = src->Colors[i].Green;
    c.b = src->Colors[i].Blue;
    c.x = 0xff;
    dest.push_back(c);
  }
}

bool GifHelper::LoadGifMetaData(GifFileType* file)
{
  if (DGifSlurp(m_gif) == GIF_ERROR)
  {
#if GIFLIB_MAJOR >= 5
    const char* error = GifErrorString(m_gif->Error);
    if (error)
      fprintf(stderr, "Gif::LoadGif(): Could not read file %s - %s\n", m_filename.c_str(), error);
#else
    int error = GifLastError();
    if (error)
      fprintf(stderr, "Gif::LoadGif(): Could not read file %s - %d\n", m_filename.c_str(), error);
#endif
    else
      fprintf(stderr, "Gif::LoadGif(): Could not read file %s (reasons unknown)\n", m_filename.c_str());
    return false;
  }

  m_height = m_gif->SHeight;
  m_width  = m_gif->SWidth;
  if (!m_height || !m_width)
  {
    fprintf(stderr, "Gif::LoadGif(): Zero sized image. File %s\n", m_filename.c_str());
    return false;
  }

  m_numFrames = m_gif->ImageCount;
  if (m_numFrames > 0)
  {
#if GIFLIB_MAJOR >= 5
    GraphicsControlBlock GCB;
    DGifSavedExtensionToGCB(m_gif, 0, &GCB);
    ExtensionBlock* extb = m_gif->SavedImages[0].ExtensionBlocks;
    if (extb && extb->Function == APPLICATION_EXT_FUNC_CODE)
    {
      // Read number of loops
      if(++extb && extb->Function == CONTINUE_EXT_FUNC_CODE)
      {
        m_loops = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1],extb->Bytes[2]);
      }
    }
#endif
  }
  else
  {
    fprintf(stderr, "Gif::LoadGif(): No images found in file %s\n", m_filename.c_str());
    return false;
  }

  m_pitch     = m_width * sizeof(COLOR);
  m_imageSize = m_pitch * m_height;
  unsigned long memoryUsage = m_numFrames * m_imageSize;
  if (memoryUsage > GIF_MAX_MEMORY)
  {
    // at least 1 image
    m_numFrames = std::max(1U, GIF_MAX_MEMORY / m_imageSize);
    fprintf(stderr, "Gif::LoadGif(): Memory consumption too high: %lu bytes. Restricting animation to %u. File %s\n", memoryUsage, m_numFrames, m_filename.c_str());
  }

  return true;
}

bool GifHelper::LoadGifMetaData(const char* file)
{
  int err = 0;
  
  if (m_gifFile.Open(file))
#if GIFLIB_MAJOR >= 5
    m_gif = DGifOpen(&m_gifFile, ReadFromVfs, &err);
#else
    m_gif = DGifOpen(&m_gifFile, ReadFromVfs);
#endif

  if (!m_gif)
  {
#if GIFLIB_MAJOR >= 5
    const char* error = GifErrorString(err);
    if (error)
      fprintf(stderr, "Gif::LoadGif(): Could not open file %s - %s\n", m_filename.c_str(), error);
#else
    int error = GifLastError();
    if (error)
      fprintf(stderr, "Gif::LoadGif(): Could not open file %s - %d\n", m_filename.c_str(), error);
#endif
    else
      fprintf(stderr, "Gif::LoadGif(): Could not open file %s (reasons unknown)\n", m_filename.c_str());
    return false;
  }
  return LoadGifMetaData(m_gif);
}

bool GifHelper::LoadGif(const char* file)
{
  m_filename = file;
  if (!LoadGifMetaData(m_filename.c_str()))
    return false;

  try
  {
    InitTemplateAndColormap();

    return ExtractFrames(m_numFrames);
  }
  catch (std::bad_alloc& ba)
  {
    fprintf(stderr, "Gif::Load(): Out of memory while reading file %s - %s\n", m_filename.c_str(), ba.what());
    Release();
    return false;
  }
}

bool GifHelper::IsAnimated(const char* file)
{
  if (m_isAnimated < 0)
  {
    m_isAnimated = 0;

    GifFileType *gif = NULL;
    FILE *gifFile;
    int err = 0;
    gifFile = fopen(file, "rb");
    if (gifFile != NULL)
    {
#if GIFLIB_MAJOR >= 5
      gif = DGifOpen(&gifFile, ReadFromVfs, &err);
#else
      gif = DGifOpen(&gifFile, ReadFromVfs);
#endif
    }

    if (gif)
    {
      if (DGifSlurp(gif) && gif->ImageCount > 1)
        m_isAnimated = 1;
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
      DGifCloseFile(gif, NULL);
#else
      DGifCloseFile(gif);
#endif
      fclose(gifFile);
    }
  }
  return m_isAnimated > 0;
}

void GifHelper::InitTemplateAndColormap()
{
  m_pTemplate = new unsigned char[m_imageSize];
  memset(m_pTemplate, 0, m_imageSize);

  if (m_gif->SColorMap)
  {
    m_globalPalette.clear();
    ConvertColorTable(m_globalPalette, m_gif->SColorMap, m_gif->SColorMap->ColorCount);

    // draw the canvas
    *m_backColor = m_globalPalette[m_gif->SBackGroundColor];
    m_hasBackground = true;

    for (unsigned int i = 0; i < m_height * m_width; ++i)
    {
      unsigned char *dest = m_pTemplate + (i *sizeof(COLOR));
      memcpy(dest, m_backColor, sizeof(COLOR));
    }
  }
  else
    m_globalPalette.clear();
}

bool GifHelper::gcbToFrame(GifFrame &frame, unsigned int imgIdx)
{
  int transparent = -1;
  frame.m_delay = 0;
  frame.m_disposal = 0;
#if GIFLIB_MAJOR >= 5
  if (m_gif->ImageCount > 0)
  {
    GraphicsControlBlock gcb;
    if (!DGifSavedExtensionToGCB(m_gif, imgIdx, &gcb))
    {
      const char* error = GifErrorString(m_gif->Error);
      if (error)
        fprintf(stderr, "Gif::ExtractFrames(): Could not read GraphicsControlBlock of frame %d - %s\n", imgIdx, error);
      else
        fprintf(stderr, "Gif::ExtractFrames(): Could not read GraphicsControlBlock of frame %d (reasons unknown)\n", imgIdx);
      return false;
    }
    // delay in ms
    frame.m_delay = gcb.DelayTime * 10;
    frame.m_disposal = gcb.DisposalMode;
    transparent = gcb.TransparentColor;
  }
#else
  if (m_gif->ImageCount > 0)
  {
    ExtensionBlock* extb = m_gif->SavedImages[imgIdx].ExtensionBlocks;
    while (extb && extb->Function != GRAPHICS_EXT_FUNC_CODE)
      extb++;

    if (extb)
    {
      frame.m_delay = UNSIGNED_LITTLE_ENDIAN(extb->Bytes[1], extb->Bytes[2]) * 10;
      frame.m_disposal = (extb->Bytes[0] >> 2) & 0x7;
      if (extb->Bytes[0] & 0x1)
        transparent = extb->Bytes[3];
      else
        transparent = -1;
    }
  }
#endif

  if (transparent >= 0 && (unsigned)transparent < frame.m_palette.size())
    frame.m_palette[transparent].x = 0;
  return true;
}

bool GifHelper::ExtractFrames(unsigned int count)
{
  if (!m_gif)
    return false;

  if (!m_pTemplate)
  {
    fprintf(stderr, "Gif::ExtractFrames(): No frame template available\n");
    return false;
  }

  for (unsigned int i = 0; i < count; i++)
  {
    GifFrame frame;
    SavedImage savedImage = m_gif->SavedImages[i];
    GifImageDesc imageDesc = m_gif->SavedImages[i].ImageDesc;
    frame.m_height = imageDesc.Height;
    frame.m_width = imageDesc.Width;
    frame.m_top = imageDesc.Top;
    frame.m_left = imageDesc.Left;
    frame.m_pitch = m_pitch;

    if (frame.m_top + frame.m_height > m_height || frame.m_left + frame.m_width > m_width
      || !frame.m_width || !frame.m_height)
    {
      fprintf(stderr, "Gif::ExtractFrames(): Illegal frame dimensions: width: %d, height: %d, left: %d, top: %d instead of (%d,%d)\n",
        frame.m_width, frame.m_height, frame.m_left, frame.m_top, m_width, m_height);
      return false;
    }

    if (imageDesc.ColorMap)
    {
      frame.m_palette.clear();
      ConvertColorTable(frame.m_palette, imageDesc.ColorMap, imageDesc.ColorMap->ColorCount);
      // TODO save a backup of the palette for frames without a table in case there's no gloabl table.
    }
    else if (m_gif->SColorMap)
    {
      frame.m_palette = m_globalPalette;
    }

    // fill delay, disposal and transparent color into frame
    if (!gcbToFrame(frame, i))
      return false;

    frame.m_pImage = new unsigned char[m_imageSize];
    frame.m_imageSize = m_imageSize;
    memcpy(frame.m_pImage, m_pTemplate, m_imageSize);

    ConstructFrame(frame, savedImage.RasterBits);

    if(!PrepareTemplate(frame))
      return false;

    m_frames.push_back(frame);
  }
  return true;
}

void GifHelper::ConstructFrame(GifFrame &frame, const unsigned char* src) const
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = frame.m_pImage + (dest_y * m_pitch) + (frame.m_left * sizeof(COLOR));

    const unsigned char *from = src + (src_y * frame.m_width);
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      COLOR col = frame.m_palette[*from++];
      if (col.x != 0)
      {
        *to++ = col.b;
        *to++ = col.g;
        *to++ = col.r;
        *to++ = col.x;
      }
      else
      {
        to += 4;
      }
    }
  }
}

bool GifHelper::PrepareTemplate(const GifFrame &frame)
{
#if GIFLIB_MAJOR >= 5
  switch (frame.m_disposal)
  {
    /* No disposal specified. */
  case DISPOSAL_UNSPECIFIED:
    /* Leave image in place */
  case DISPOSE_DO_NOT:
    memcpy(m_pTemplate, frame.m_pImage, m_imageSize);
    break;

    /* Set area too background color */
  case DISPOSE_BACKGROUND:
    {
      if (!m_hasBackground)
      {
        fprintf(stderr, "Gif::PrepareTemplate(): Disposal method DISPOSE_BACKGROUND encountered, but the gif has no background.\n");
        return false;
      }
      SetFrameAreaToBack(m_pTemplate, frame);
      break;
    }
    /* Restore to previous content */
  case DISPOSE_PREVIOUS:
    {
      bool valid = false;

      for (int i = m_frames.size() - 1 ; i >= 0; --i)
      {
        if (m_frames[i].m_disposal != DISPOSE_PREVIOUS)
        {
          memcpy(m_pTemplate, m_frames[i].m_pImage, m_imageSize);
          valid = true;
          break;
        }
      }
      if (!valid)
      {
        fprintf(stderr, "Gif::PrepareTemplate(): Disposal method DISPOSE_PREVIOUS encountered, but could not find a suitable frame.\n");
        return false;
      }
      break;
    }
  default:
    {
      fprintf(stderr, "Gif::PrepareTemplate(): Unknown disposal method: %d\n", frame.m_disposal);
      return false;
    }
  }
#endif
  return true;
}

void GifHelper::SetFrameAreaToBack(unsigned char* dest, const GifFrame &frame)
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = dest + (dest_y * m_pitch) + (frame.m_left * sizeof(COLOR));
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      memcpy(to, m_backColor, sizeof(COLOR));
      to += 4;
    }
  }
}

bool GifHelper::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)
{
  if (!buffer || !bufSize || !width || !height)
    return false;

  Gifreader reader;
  reader.buffer = buffer;
  reader.buffSize = bufSize;

  int err = 0;
#if GIFLIB_MAJOR >= 5
  m_gif = DGifOpen((void *)&reader, (InputFunc)&ReadFromMemory, &err);
#else
  m_gif = DGifOpen((void *)&reader, (InputFunc)&ReadFromMemory);
#endif
  if (!m_gif)
  {
#if GIFLIB_MAJOR >= 5
    const char* error = GifErrorString(err);
    if (error)
      fprintf(stderr, "Gif::LoadImageFromMemory(): Could not open gif from memory - %s\n", error);
#else
    int error = GifLastError();
    if (error)
      fprintf(stderr, "Gif::LoadImageFromMemory(): Could not open gif from memory - %d\n", error);
#endif
    else
      fprintf(stderr, "Gif::LoadImageFromMemory(): Could not open gif from memory (reasons unknown)\n");
    return false;
  }

  if (!LoadGifMetaData(m_gif))
    return false;

  m_originalWidth = m_width;
  m_originalHeight = m_height;

  try
  {
    InitTemplateAndColormap();

    if (!ExtractFrames(m_numFrames))
      return false;
  }
  catch (std::bad_alloc& ba)
  {
    fprintf(stderr, "Gif::LoadImageFromMemory(): Out of memory while extracting gif frames - %s\n", ba.what());
    Release();
    return false;
  }

  return true;
}

GifFrame::GifFrame() :
  m_pImage(NULL),
  m_delay(0),
  m_imageSize(0),
  m_height(0),
  m_width(0),
  m_top(0),
  m_left(0),
  m_disposal(0),
  m_transparent(0),
  m_pitch(0)
{}


GifFrame::GifFrame(const GifFrame& src) :
  m_pImage(NULL),
  m_delay(src.m_delay),
  m_imageSize(src.m_imageSize),
  m_height(src.m_height),
  m_width(src.m_width),
  m_top(src.m_top),
  m_left(src.m_left),
  m_disposal(src.m_disposal),
  m_transparent(src.m_transparent),
  m_pitch(src.m_pitch)
{
  if (src.m_pImage)
  {
    m_pImage = new unsigned char[m_imageSize];
    memcpy(m_pImage, src.m_pImage, m_imageSize);
  }

  if (src.m_palette.size())
  {
    m_palette = src.m_palette;
  }
}

GifFrame::~GifFrame()
{
  delete[] m_pImage;
  m_pImage = NULL;
}
