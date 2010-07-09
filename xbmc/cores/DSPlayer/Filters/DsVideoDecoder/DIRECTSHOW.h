#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "XBMCVideoDecFilter.h"
#include "DVDPlayer/DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "Event.h"
#include "utils/CriticalSection.h"


namespace DIRECTSHOW {
#define NUM_OUTPUT_SURFACES                4
#define NUM_VIDEO_SURFACES_MPEG2           10  // (1 frame being decoded, 2 reference)
#define NUM_VIDEO_SURFACES_H264            32 // (1 frame being decoded, up to 16 references)
#define NUM_VIDEO_SURFACES_VC1             10  // (same as MPEG-2)

class CDecoder
  : public CXBMCVideoDecFilter::IDXVADecoder
{
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, directshow_dxva_h264* picture);
  virtual int  Check     (AVCodecContext* avctx);
  virtual void Close();

  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void  RelBuffer(AVCodecContext *avctx, AVFrame *pic);
  

  static bool      Supports(enum PixelFormat fmt);
  
  struct SVideoBuffer
  {
    SVideoBuffer();
   ~SVideoBuffer();
    void Init(int index);
    void Clear();

    directshow_dxva_h264* surface;
    int                surface_index;
    bool               used;
    int                age;
  };

  static const unsigned        m_buffer_max = 32;
  SVideoBuffer                 m_buffer[m_buffer_max];
  unsigned                     m_buffer_count;
  unsigned                     m_buffer_age;
  int                          m_refs;
  CCriticalSection             m_section;

  /*std::vector<directshow_dxva_h264*> m_videoBuffer;*/
  
};

};/* Namespace DIRECTSHOW */