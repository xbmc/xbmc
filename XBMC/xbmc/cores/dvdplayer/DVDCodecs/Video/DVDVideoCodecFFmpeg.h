#pragma once
#ifndef __DVDVIDEOCODECFFMMPEG_H
#define __DVDVIDEOCODECFFMMPEG_H
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDVideoCodec.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllSwScale.h"
#include "utils/SharedSection.h"

#ifdef HAVE_LIBVDPAU
#include "DVDVideoCodecFFmpegVDPAU.h"
extern CCriticalSection g_VDPAUSection;
extern CDVDVideoCodecVDPAU* g_VDPAU;
#endif

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);  
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double pts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return "FFmpeg"; };

#ifdef HAVE_LIBVDPAU
  CDVDVideoCodecVDPAU* GetContextVDPAU();
#endif
  
protected:

  void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);

  AVFrame* m_pFrame;
  AVCodecContext* m_pCodecContext;
  
  AVFrame* m_pConvertFrame;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;

  DllAvCodec m_dllAvCodec;
  DllAvUtil  m_dllAvUtil;
  DllSwScale m_dllSwScale;
};

#endif  //__DVDVIDEOCODECFFMMPEG_H


