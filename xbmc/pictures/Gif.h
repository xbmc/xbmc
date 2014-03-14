#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://www.xbmc.org
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

#include "guilib/iimage.h"
#include "DllLibGif.h"
#include "guilib/Texture.h"

class GifFrame 
{
  friend class Gif;
public: 

  GifFrame();
  virtual ~GifFrame();
  void Release();

  GifFrame(const GifFrame& src);

  unsigned char*  m_pImage;
  unsigned int    m_delay;

private:

  unsigned int    m_imageSize;
  unsigned int    m_ColorTableSize;
  unsigned int    m_height;
  unsigned int    m_width;
  unsigned int    m_top;
  unsigned int    m_left;
  COLOR* m_pPalette;
  unsigned int m_disposal;
  int m_transparent;
};



class Gif : public IImage
{
  friend class GifFrame;
public:
  Gif();
  virtual ~Gif();

  bool LoadGifMetaData(const char* file);
  bool LoadGif(const char* file);

  std::vector<GifFrame> m_frames;
  unsigned int    m_imageSize;
  unsigned int    m_pitch;
  unsigned int    m_loops;
  unsigned int    m_numFrames;

  virtual bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height);
  virtual bool Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format);
  virtual bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const CStdString& destFile, 
                                          unsigned char* &bufferout, unsigned int &bufferoutSize);

private:
  DllLibGif       m_dll;
  std::string     m_filename;
  GifFileType*    m_gif;
  bool            m_hasBackground;
  COLOR           m_backColor;
  COLOR*          m_pGlobalPalette;
  unsigned int    m_gloabalPaletteSize;
  unsigned char*  m_pTemplate;
  unsigned int    m_optimalWidth;
  unsigned int    m_optimalHeight;

  void InitTemplateAndColormap();
  bool LoadGifMetaData(GifFileType* file);
  static void ConvertColorTable(COLOR* dest, ColorMapObject* src, unsigned int size);  
  bool ExtractFrames(unsigned int count);
  void SetFrameAreaToBack(unsigned char* dest, const GifFrame &frame);
  void ConstructFrame(GifFrame &frame, const unsigned char* src) const;
  bool PrepareTemplate(const GifFrame &frame);
  void Release();
};
