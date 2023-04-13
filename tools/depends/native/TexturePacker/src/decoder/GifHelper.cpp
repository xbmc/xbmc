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
#include <cstdlib>
#include <cstring>

#define UNSIGNED_LITTLE_ENDIAN(lo, hi)	((lo) | ((hi) << 8))
#define GIF_MAX_MEMORY 82944000U // about 79 MB, which is equivalent to 10 full hd frames.

class Gifreader
{
public:
  unsigned char* buffer = nullptr;
  unsigned int buffSize = 0;
  unsigned int readPosition = 0;

  Gifreader() = default;
};

int ReadFromVfs(GifFileType* gif, GifByteType* gifbyte, int len)
{
  CFile *gifFile = static_cast<CFile*>(gif->UserData);
  return gifFile->Read(gifbyte, len);
}

GifHelper::~GifHelper()
{
    Close(m_gif);
    Release();
}

bool GifHelper::Open(GifFileType*& gif, void *dataPtr, InputFunc readFunc)
{
  int err = 0;
#if GIFLIB_MAJOR == 5
  gif = DGifOpen(dataPtr, readFunc, &err);
#else
  gif = DGifOpen(dataPtr, readFunc);
  if (!gif)
    err = GifLastError();
#endif

  if (!gif)
  {
    fprintf(stderr, "Gif::Open(): Could not open file %s. Reason: %s\n", m_filename.c_str(), GifErrorString(err));
    return false;
  }

  return true;
}

void GifHelper::Close(GifFileType* gif)
{
  int err = 0;
  int reason = 0;
#if GIFLIB_MAJOR == 5 && GIFLIB_MINOR >= 1
  err = DGifCloseFile(gif, &reason);
#else
  err = DGifCloseFile(gif);
#if GIFLIB_MAJOR < 5
  reason = GifLastError();
#endif
  if (err == GIF_ERROR)
    free(gif);
#endif
  if (err == GIF_ERROR)
  {
    fprintf(stderr, "GifHelper::Close(): closing file %s failed. Reason: %s\n", m_filename.c_str(), Reason(reason));
  }
}

const char* GifHelper::Reason(int reason)
{
  const char* err = GifErrorString(reason);
  if (err)
    return err;

  return "unknown";

}

void GifHelper::Release()
{
  m_pTemplate.clear();
  m_globalPalette.clear();
  m_frames.clear();
}

void GifHelper::ConvertColorTable(std::vector<GifColor> &dest, ColorMapObject* src, unsigned int size)
{
  for (unsigned int i = 0; i < size; ++i)
  {
    GifColor c;

    c.r = src->Colors[i].Red;
    c.g = src->Colors[i].Green;
    c.b = src->Colors[i].Blue;
    c.a = 0xff;
    dest.push_back(c);
  }
}

bool GifHelper::LoadGifMetaData(GifFileType* gif)
{
  if (!Slurp(gif))
    return false;

  m_height = gif->SHeight;
  m_width = gif->SWidth;
  if (!m_height || !m_width)
  {
    fprintf(stderr, "Gif::LoadGif(): Zero sized image. File %s\n", m_filename.c_str());
    return false;
  }

  m_numFrames = gif->ImageCount;
  if (m_numFrames > 0)
  {
    ExtensionBlock* extb = gif->SavedImages[0].ExtensionBlocks;
    if (extb && extb->Function == APPLICATION_EXT_FUNC_CODE)
    {
      // Read number of loops
      if (++extb && extb->Function == CONTINUE_EXT_FUNC_CODE)
      {
        uint8_t low = static_cast<uint8_t>(extb->Bytes[1]);
        uint8_t high = static_cast<uint8_t>(extb->Bytes[2]);
        m_loops = UNSIGNED_LITTLE_ENDIAN(low, high);
      }
    }
  }
  else
  {
    fprintf(stderr, "Gif::LoadGif(): No images found in file %s\n", m_filename.c_str());
    return false;
  }

  m_pitch = m_width * sizeof(GifColor);
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

bool GifHelper::LoadGifMetaData(const std::string& file)
{
  m_gifFile.Close();
  if (!m_gifFile.Open(file) || !Open(m_gif, &m_gifFile, ReadFromVfs))
    return false;

  return LoadGifMetaData(m_gif);
}

bool GifHelper::Slurp(GifFileType* gif)
{
  if (DGifSlurp(gif) == GIF_ERROR)
  {
    int reason = 0;
#if GIFLIB_MAJOR == 5
    reason = gif->Error;
#else
    reason = GifLastError();
#endif
    fprintf(stderr, "Gif::LoadGif(): Could not read file %s. Reason: %s\n", m_filename.c_str(), GifErrorString(reason));
    return false;
  }

  return true;
}

bool GifHelper::LoadGif(const std::string& file)
{
  m_filename = file;
  if (!LoadGifMetaData(m_filename))
    return false;

  try
  {
    InitTemplateAndColormap();

    int extractedFrames = ExtractFrames(m_numFrames);
    if (extractedFrames < 0)
    {
      fprintf(stderr, "Gif::LoadGif(): Could not extract any frame. File %s\n", m_filename.c_str());
      return false;
    }
    else if (extractedFrames < (int)m_numFrames)
    {
      fprintf(stderr, "Gif::LoadGif(): Could only extract %d/%d frames. File %s\n", extractedFrames, m_numFrames, m_filename.c_str());
      m_numFrames = extractedFrames;
    }

    return true;
  }
  catch (std::bad_alloc& ba)
  {
    fprintf(stderr, "Gif::Load(): Out of memory while reading file %s - %s\n", m_filename.c_str(), ba.what());
    Release();
    return false;
  }
}

void GifHelper::InitTemplateAndColormap()
{
  m_pTemplate.resize(m_imageSize);

  if (m_gif->SColorMap)
  {
    m_globalPalette.clear();
    ConvertColorTable(m_globalPalette, m_gif->SColorMap, m_gif->SColorMap->ColorCount);
  }
  else
    m_globalPalette.clear();
}

bool GifHelper::GcbToFrame(GifFrame &frame, unsigned int imgIdx)
{
  int transparent = -1;
  frame.m_delay = 0;
  frame.m_disposal = 0;

  if (m_gif->ImageCount > 0)
  {
#if GIFLIB_MAJOR == 5
    GraphicsControlBlock gcb;
    if (DGifSavedExtensionToGCB(m_gif, imgIdx, &gcb))
    {
      // delay in ms
      frame.m_delay = gcb.DelayTime * 10;
      frame.m_disposal = gcb.DisposalMode;
      transparent = gcb.TransparentColor;
    }
#else
    ExtensionBlock* extb = m_gif->SavedImages[imgIdx].ExtensionBlocks;
    while (extb && extb->Function != GRAPHICS_EXT_FUNC_CODE)
      extb++;

    if (extb && extb->ByteCount == 4)
    {
      uint8_t low = static_cast<uint8_t>(extb->Bytes[1]);
      uint8_t high = static_cast<uint8_t>(extb->Bytes[2]);
      frame.m_delay = UNSIGNED_LITTLE_ENDIAN(low, high) * 10;
      frame.m_disposal = (extb->Bytes[0] >> 2) & 0x07;
      if (extb->Bytes[0] & 0x01)
      {
        transparent = static_cast<uint8_t>(extb->Bytes[3]);
      }
      else
        transparent = -1;
    }
#endif
  }

  if (transparent >= 0 && (unsigned)transparent < frame.m_palette.size())
    frame.m_palette[transparent].a = 0;
  return true;
}

int GifHelper::ExtractFrames(unsigned int count)
{
  if (!m_gif)
    return -1;

  int extracted = 0;
  for (unsigned int i = 0; i < count; i++)
  {
    FramePtr frame(new GifFrame);
    SavedImage savedImage = m_gif->SavedImages[i];
    GifImageDesc imageDesc = m_gif->SavedImages[i].ImageDesc;
    frame->m_height = imageDesc.Height;
    frame->m_width = imageDesc.Width;
    frame->m_top = imageDesc.Top;
    frame->m_left = imageDesc.Left;

    if (frame->m_top + frame->m_height > m_height || frame->m_left + frame->m_width > m_width
      || !frame->m_width || !frame->m_height
      || frame->m_width > m_width || frame->m_height > m_height)
    {
      fprintf(stderr, "Gif::ExtractFrames(): Illegal frame dimensions: width: %d, height: %d, left: %d, top: %d instead of (%d,%d), skip it\n",
        frame->m_width, frame->m_height, frame->m_left, frame->m_top, m_width, m_height);
      continue;
    }

    if (imageDesc.ColorMap)
    {
      frame->m_palette.clear();
      ConvertColorTable(frame->m_palette, imageDesc.ColorMap, imageDesc.ColorMap->ColorCount);
      // TODO save a backup of the palette for frames without a table in case there's no global table.
    }
    else if (m_gif->SColorMap)
    {
      frame->m_palette = m_globalPalette;
    }
    else
    {
      fprintf(stderr, "Gif::ExtractFrames(): No color map found for frame %d, skip it\n", i);
      continue;
    }

    // fill delay, disposal and transparent color into frame
    if (!GcbToFrame(*frame, i))
    {
      fprintf(stderr, "Gif::ExtractFrames(): Corrupted Graphics Control Block for frame %d, skip it\n", i);
      continue;
    }

    frame->m_pImage = m_pTemplate;

    ConstructFrame(*frame, savedImage.RasterBits);

    if (!PrepareTemplate(*frame))
    {
      fprintf(stderr, "Gif::ExtractFrames(): Could not prepare template after frame %d, skip it\n", i);
      continue;
    }

    extracted++;
    m_frames.push_back(frame);
  }
  return extracted;
}

void GifHelper::ConstructFrame(GifFrame &frame, const unsigned char* src) const
{
  size_t paletteSize = frame.m_palette.size();

  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char* to =
        frame.m_pImage.data() + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));

    const unsigned char *from = src + (src_y * frame.m_width);
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      unsigned char index = *from++;

      if (index >= paletteSize)
      {
        fprintf(stderr, "Gif::ConstructFrame(): Pixel (%d,%d) has no valid palette entry, skip it\n", src_x, src_y);
        continue;
      }

      GifColor col = frame.m_palette[index];
      if (col.a != 0)
        memcpy(to, &col, sizeof(GifColor));

      to += 4;
    }
  }
}

bool GifHelper::PrepareTemplate(GifFrame &frame)
{
  switch (frame.m_disposal)
  {
    /* No disposal specified. */
  case DISPOSAL_UNSPECIFIED:
    /* Leave image in place */
  case DISPOSE_DO_NOT:
      m_pTemplate = frame.m_pImage;
      break;

      /*
       Clear the frame's area to transparency.
       The disposal names is misleading. Do not restore to the background color because
       this part of the specification is ignored by all browsers/image viewers.
    */
  case DISPOSE_BACKGROUND:
  {
      ClearFrameAreaToTransparency(m_pTemplate.data(), frame);
      break;
  }
  /* Restore to previous content */
  case DISPOSE_PREVIOUS:
  {

    /*
    * This disposal method makes no sense for the first frame
    * Since browsers etc. handle that too, we'll fall back to DISPOSE_DO_NOT
    */
    if (m_frames.empty())
    {
      frame.m_disposal = DISPOSE_DO_NOT;
      return PrepareTemplate(frame);
    }

    bool valid = false;

    for (int i = m_frames.size() - 1; i >= 0; --i)
    {
      if (m_frames[i]->m_disposal != DISPOSE_PREVIOUS)
      {
        m_pTemplate = m_frames[i]->m_pImage;
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
    fprintf(stderr, "Gif::PrepareTemplate(): Unknown disposal method: %d. Using DISPOSAL_UNSPECIFIED, the animation might be wrong now.\n", frame.m_disposal);
    frame.m_disposal = DISPOSAL_UNSPECIFIED;
    return PrepareTemplate(frame);
  }
  }
  return true;
}

void GifHelper::ClearFrameAreaToTransparency(unsigned char* dest, const GifFrame &frame)
{
  for (unsigned int dest_y = frame.m_top, src_y = 0; src_y < frame.m_height; ++dest_y, ++src_y)
  {
    unsigned char *to = dest + (dest_y * m_pitch) + (frame.m_left * sizeof(GifColor));
    for (unsigned int src_x = 0; src_x < frame.m_width; ++src_x)
    {
      to += 3;
      *to++ = 0;
    }
  }
}
