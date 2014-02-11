#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#if defined(HAVE_VIDEOTOOLBOXDECODER)

#include <queue>

#include "DVDVideoCodec.h"
#include <CoreVideo/CoreVideo.h>
#include <CoreMedia/CoreMedia.h>

// tracks a frame in and output queue in display order
typedef struct frame_queue {
  double              dts;
  double              pts;
  int                 width;
  int                 height;
  double              sort_time;
  FourCharCode        pixel_buffer_format;
  CVPixelBufferRef    pixel_buffer_ref;
  struct frame_queue  *nextframe;
} frame_queue;

class CDVDVideoCodecVideoToolBox : public CDVDVideoCodec
{
public:
  CDVDVideoCodecVideoToolBox();
  virtual ~CDVDVideoCodecVideoToolBox();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }

protected:
  void DisplayQueuePop(void);
  void CreateVTSession(int width, int height, CMFormatDescriptionRef fmt_desc);
  void DestroyVTSession(void);
  static void VTDecoderCallback(
    void *refcon, CFDictionaryRef frameInfo,
    OSStatus status, UInt32 infoFlags, CVBufferRef imageBuffer);

  void              *m_vt_session;    // opaque videotoolbox session
  CMFormatDescriptionRef m_fmt_desc;

  const char        *m_pFormatName;
  bool              m_DropPictures;
  DVDVideoPicture   m_videobuffer;

  double            m_sort_time_offset;
  pthread_mutex_t   m_queue_mutex;    // mutex protecting queue manipulation
  frame_queue       *m_display_queue; // display-order queue - next display frame is always at the queue head
  int32_t           m_queue_depth;    // we will try to keep the queue depth at m_max_ref_frames
  int32_t           m_max_ref_frames;

  bool              m_convert_bytestream;
  bool              m_convert_3byteTo4byteNALSize;
};

#endif
