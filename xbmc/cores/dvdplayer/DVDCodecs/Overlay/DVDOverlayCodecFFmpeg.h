#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef OVERLAY_DVDOVERLAYCODEC_H_INCLUDED
#define OVERLAY_DVDOVERLAYCODEC_H_INCLUDED
#include "DVDOverlayCodec.h"
#endif

#ifndef OVERLAY_DLLAVCODEC_H_INCLUDED
#define OVERLAY_DLLAVCODEC_H_INCLUDED
#include "DllAvCodec.h"
#endif

#ifndef OVERLAY_DLLAVUTIL_H_INCLUDED
#define OVERLAY_DLLAVUTIL_H_INCLUDED
#include "DllAvUtil.h"
#endif


class CDVDOverlaySpu;
class CDVDOverlayText;

class CDVDOverlayCodecFFmpeg : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecFFmpeg();
  virtual ~CDVDOverlayCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(DemuxPacket *pPacket);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();

private:
  void FreeSubtitle(AVSubtitle &sub);

  AVCodecContext* m_pCodecContext;
  AVSubtitle      m_Subtitle;
  int             m_SubtitleIndex;
  double          m_StartTime;
  double          m_StopTime;

  int             m_width;
  int             m_height;

  DllAvCodec      m_dllAvCodec;
  DllAvUtil       m_dllAvUtil;

};
