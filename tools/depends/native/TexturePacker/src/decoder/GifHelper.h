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

#include "gif_lib.h"
#include <vector>
#include <string>
#include "SimpleFS.h"


#pragma pack(1)
struct COLOR { unsigned char b, g, r, x; };	// Windows GDI expects 4bytes per color
#pragma pack()

class CFile;

class GifFrame
{
  friend class GifHelper;
public:

  GifFrame();
  virtual ~GifFrame();
  void Release();

  GifFrame(const GifFrame& src);

  unsigned char*  m_pImage;
  unsigned int    m_delay;
  unsigned int    m_top;
  unsigned int    m_left;
  unsigned int    m_disposal;
  unsigned int    m_height;
  unsigned int    m_width;
  unsigned int    m_pitch;
  unsigned int    m_imageSize;

private:

  std::vector<COLOR>   m_palette;
  int m_transparent;
};



class GifHelper
{
  friend class GifFrame;
public:
  GifHelper();
  virtual ~GifHelper();

  bool LoadGifMetaData(const char* file);
  bool LoadGif(const char* file);

  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height);
  //virtual bool Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format);
  bool IsAnimated(const char* file);
  std::vector<GifFrame>& GetFrames() { return m_frames; }
  unsigned int GetPitch() const { return m_pitch; }
  unsigned int GetNumLoops() const { return m_loops; }

private:
  std::vector<GifFrame> m_frames;
  unsigned int    m_imageSize;
  unsigned int    m_pitch;
  unsigned int    m_loops;
  unsigned int    m_numFrames;

  std::string     m_filename;
  GifFileType*    m_gif;
  bool            m_hasBackground;
  COLOR*          m_backColor;
  std::vector<COLOR> m_globalPalette;
  unsigned char*  m_pTemplate;
  int             m_isAnimated;
  CFile           m_gifFile;

  unsigned int m_width;
  unsigned int m_height;
  unsigned int m_originalWidth;   ///< original image width before scaling or cropping
  unsigned int m_originalHeight;  ///< original image height before scaling or cropping
  unsigned int m_orientation;
  bool m_hasAlpha;

  void InitTemplateAndColormap();
  bool LoadGifMetaData(GifFileType* file);
  static void ConvertColorTable(std::vector<COLOR> &dest, ColorMapObject* src, unsigned int size);
  bool gcbToFrame(GifFrame &frame, unsigned int imgIdx);
  bool ExtractFrames(unsigned int count);
  void SetFrameAreaToBack(unsigned char* dest, const GifFrame &frame);
  void ConstructFrame(GifFrame &frame, const unsigned char* src) const;
  bool PrepareTemplate(const GifFrame &frame);
  void Release();
};
