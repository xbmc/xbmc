#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#if defined(HAVE_LIBVDADECODER)
#include <queue>

#include "DVDVideoCodec.h"
#include "Codecs/DllSwScale.h"
#include <CoreVideo/CoreVideo.h>

// tracks a frame in and output queue in display order
typedef struct frame_queue {
  double              frametime;
  CVPixelBufferRef    frame;
  struct frame_queue  *nextframe;
} frame_queue;

class DllLibVDADecoder;
class CDVDVideoCodecVDA : public CDVDVideoCodec
{
public:
  CDVDVideoCodecVDA();
  virtual ~CDVDVideoCodecVDA();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(BYTE *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  
protected:
  void DisplayQueuePop(void);
  void UYVY422_to_YUV420P(uint8_t *yuv422_ptr, int yuv422_stride, DVDVideoPicture *picture);

  static void VDADecoderCallback(
    void *decompressionOutputRefCon, CFDictionaryRef frameInfo,
    OSStatus status, uint32_t infoFlags, CVImageBufferRef imageBuffer);

  DllLibVDADecoder  *m_dll;
  void              *m_vda_decoder;   // opaque vdadecoder reference
  int32_t           m_format;
  const char        *m_pFormatName;
  bool              m_DropPictures;

  pthread_mutex_t   m_queue_mutex;    // mutex protecting queue manipulation
  frame_queue       *m_display_queue; // display-order queue - next display frame is always at the queue head
  int32_t           m_queue_depth;    // we will try to keep the queue depth around 16+1 frames
  std::queue<double> m_dts_queue;

  DllSwScale        m_dllSwScale;
  struct SwsContext *m_swcontext;
  DVDVideoPicture   m_videobuffer;
};

#endif
