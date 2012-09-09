#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if defined(HAVE_LIBVDADECODER)

#include "DVDVideoCodec.h"
#include <CoreVideo/CoreVideo.h>

// tracks a frame in and output queue in display order
typedef struct frame_queue {
  double              dts;
  double              pts;
  double              sort_time;
  FourCharCode        pixel_buffer_format;
  CVBufferRef         pixel_buffer_ref;
  struct frame_queue  *nextframe;
} frame_queue;

class DllAvUtil;
class DllSwScale;
class DllAvFormat;
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
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  
protected:
  void DisplayQueuePop(void);

  static void VDADecoderCallback(
    void *decompressionOutputRefCon, CFDictionaryRef frameInfo,
    OSStatus status, uint32_t infoFlags, CVImageBufferRef imageBuffer);

  DllLibVDADecoder  *m_dll;
  void              *m_vda_decoder;   // opaque vdadecoder reference
  int32_t           m_format;
  const char        *m_pFormatName;
  bool              m_DropPictures;

  double            m_sort_time_offset;
  pthread_mutex_t   m_queue_mutex;    // mutex protecting queue manipulation
  frame_queue       *m_display_queue; // display-order queue - next display frame is always at the queue head
  int32_t           m_queue_depth;    // we will try to keep the queue depth around 16+1 frames
  int32_t           m_max_ref_frames;
  
  bool              m_convert_bytestream;
  bool              m_convert_3byteTo4byteNALSize;
  DllAvUtil         *m_dllAvUtil;
  DllAvFormat       *m_dllAvFormat;
};

#endif
