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

#pragma once

#include <gif_lib.h>
#ifndef CONTINUE_EXT_FUNC_CODE
#define CONTINUE_EXT_FUNC_CODE 0
#endif

#ifndef DISPOSAL_UNSPECIFIED
#define DISPOSAL_UNSPECIFIED 0
#endif

#ifndef DISPOSE_DO_NOT
#define DISPOSE_DO_NOT 1
#endif

#ifndef DISPOSE_BACKGROUND
#define DISPOSE_BACKGROUND 2
#endif

#ifndef DISPOSE_PREVIOUS
#define DISPOSE_PREVIOUS 3
#endif

#include <vector>
#include <string>
#include <memory>
#include "SimpleFS.h"

#pragma pack(1)
struct GifColor
{
  uint8_t b, g, r, a;
};
#pragma pack()

class CFile;

class GifFrame
{
  friend class GifHelper;
public:

  GifFrame() = default;
  virtual ~GifFrame() = default;

  std::vector<uint8_t> m_pImage;
  unsigned int m_delay = 0;

private:
  unsigned int m_top = 0;
  unsigned int m_left = 0;
  unsigned int m_disposal = 0;
  unsigned int m_height = 0;
  unsigned int m_width = 0;
  std::vector<GifColor>   m_palette;
};



class GifHelper
{
  friend class GifFrame;

  typedef std::shared_ptr<GifFrame> FramePtr;

public:
  GifHelper() = default;
  virtual ~GifHelper();

  bool LoadGif(const std::string& file);

  std::vector<FramePtr>& GetFrames() { return m_frames; }
  unsigned int GetPitch() const { return m_pitch; }
  unsigned int GetNumLoops() const { return m_loops; }
  unsigned int GetWidth() const { return m_width; }
  unsigned int GetHeight() const { return m_height; }

private:
  std::vector<FramePtr> m_frames;
  unsigned int m_imageSize = 0;
  unsigned int m_pitch = 0;
  unsigned int m_loops = 0;
  unsigned int m_numFrames = 0;

  std::string     m_filename;
  GifFileType* m_gif = nullptr;
  std::vector<GifColor> m_globalPalette;
  std::vector<uint8_t> m_pTemplate;
  CFile m_gifFile;

  unsigned int m_width;
  unsigned int m_height;

  bool Open(GifFileType *& gif, void * dataPtr, InputFunc readFunc);
  void Close(GifFileType * gif);

  const char* Reason(int reason);

  bool LoadGifMetaData(const std::string& file);
  bool Slurp(GifFileType* gif);
  void InitTemplateAndColormap();
  bool LoadGifMetaData(GifFileType* gif);
  static void ConvertColorTable(std::vector<GifColor> &dest, ColorMapObject* src, unsigned int size);
  bool GcbToFrame(GifFrame &frame, unsigned int imgIdx);
  int ExtractFrames(unsigned int count);
  void ClearFrameAreaToTransparency(unsigned char* dest, const GifFrame &frame);
  void ConstructFrame(GifFrame &frame, const unsigned char* src) const;
  bool PrepareTemplate(GifFrame &frame);
  void Release();

#if GIFLIB_MAJOR != 5
  /*
  taken from giflib 5.1.0
  */
  const char* GifErrorString(int ErrorCode)
  {
    const char *Err;

    switch (ErrorCode) {
    case E_GIF_ERR_OPEN_FAILED:
      Err = "Failed to open given file";
      break;
    case E_GIF_ERR_WRITE_FAILED:
      Err = "Failed to write to given file";
      break;
    case E_GIF_ERR_HAS_SCRN_DSCR:
      Err = "Screen descriptor has already been set";
      break;
    case E_GIF_ERR_HAS_IMAG_DSCR:
      Err = "Image descriptor is still active";
      break;
    case E_GIF_ERR_NO_COLOR_MAP:
      Err = "Neither global nor local color map";
      break;
    case E_GIF_ERR_DATA_TOO_BIG:
      Err = "Number of pixels bigger than width * height";
      break;
    case E_GIF_ERR_NOT_ENOUGH_MEM:
      Err = "Failed to allocate required memory";
      break;
    case E_GIF_ERR_DISK_IS_FULL:
      Err = "Write failed (disk full?)";
      break;
    case E_GIF_ERR_CLOSE_FAILED:
      Err = "Failed to close given file";
      break;
    case E_GIF_ERR_NOT_WRITEABLE:
      Err = "Given file was not opened for write";
      break;
    case D_GIF_ERR_OPEN_FAILED:
      Err = "Failed to open given file";
      break;
    case D_GIF_ERR_READ_FAILED:
      Err = "Failed to read from given file";
      break;
    case D_GIF_ERR_NOT_GIF_FILE:
      Err = "Data is not in GIF format";
      break;
    case D_GIF_ERR_NO_SCRN_DSCR:
      Err = "No screen descriptor detected";
      break;
    case D_GIF_ERR_NO_IMAG_DSCR:
      Err = "No Image Descriptor detected";
      break;
    case D_GIF_ERR_NO_COLOR_MAP:
      Err = "Neither global nor local color map";
      break;
    case D_GIF_ERR_WRONG_RECORD:
      Err = "Wrong record type detected";
      break;
    case D_GIF_ERR_DATA_TOO_BIG:
      Err = "Number of pixels bigger than width * height";
      break;
    case D_GIF_ERR_NOT_ENOUGH_MEM:
      Err = "Failed to allocate required memory";
      break;
    case D_GIF_ERR_CLOSE_FAILED:
      Err = "Failed to close given file";
      break;
    case D_GIF_ERR_NOT_READABLE:
      Err = "Given file was not opened for read";
      break;
    case D_GIF_ERR_IMAGE_DEFECT:
      Err = "Image is defective, decoding aborted";
      break;
    case D_GIF_ERR_EOF_TOO_SOON:
      Err = "Image EOF detected before image complete";
      break;
    default:
      Err = NULL;
      break;
    }
    return Err;
  }
#endif
};
