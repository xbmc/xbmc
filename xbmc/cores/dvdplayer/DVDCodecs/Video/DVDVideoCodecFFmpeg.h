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

#include "cores/dvdplayer/DVDCodecs/DVDCodecs.h"
#include "cores/dvdplayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "DVDResource.h"
#include <string>
#include <vector>

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"
}

class CCriticalSection;

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  class IHardwareDecoder : public IDVDResourceCounted<IHardwareDecoder>
  {
    public:
             IHardwareDecoder() {}
    virtual ~IHardwareDecoder() {};
    virtual bool Open      (AVCodecContext* avctx, AVCodecContext* mainctx, const enum PixelFormat, unsigned int surfaces) = 0;
    virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame) = 0;
    virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture) = 0;
    virtual int  Check     (AVCodecContext* avctx) = 0;
    virtual void Reset     () {}
    virtual unsigned GetAllowedReferences() { return 0; }
    virtual bool CanSkipDeint() {return false; }
    virtual const std::string Name() = 0;
  };

  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts);
  virtual void Reset();
  virtual void Reopen();
  bool GetPictureCommon(DVDVideoPicture* pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual unsigned int SetFilters(unsigned int filters);
  virtual const char* GetName() { return m_name.c_str(); }; // m_name is never changed after open
  virtual unsigned GetConvergeCount();
  virtual unsigned GetAllowedReferences();
  virtual bool GetCodecStats(double &pts, int &droppedPics);
  virtual void SetCodecControl(int flags);

  IHardwareDecoder * GetHardware()                           { return m_pHardware; };
  void               SetHardware(IHardwareDecoder* hardware);

protected:
  static enum PixelFormat GetFormat(struct AVCodecContext * avctx, const PixelFormat * fmt);

  int  FilterOpen(const std::string& filters, bool scale);
  void FilterClose();
  int  FilterProcess(AVFrame* frame);
  void DisposeHWDecoders();

  void UpdateName()
  {
    if(m_pCodecContext->codec->name)
      m_name = std::string("ff-") + m_pCodecContext->codec->name;
    else
      m_name = "ffmpeg";

    if(m_pHardware)
      m_name += "-" + m_pHardware->Name();
  }

  AVFrame* m_pFrame;
  AVCodecContext* m_pCodecContext;

  std::string       m_filters;
  std::string       m_filters_next;
  AVFilterGraph*   m_pFilterGraph;
  AVFilterContext* m_pFilterIn;
  AVFilterContext* m_pFilterOut;
  AVFrame*         m_pFilterFrame;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;
  int m_iOrientation;// orientation of the video in degress counter clockwise

  unsigned int m_uSurfacesCount;

  std::string m_name;
  int m_decoderState;
  IHardwareDecoder *m_pHardware;
  std::vector<IHardwareDecoder*> m_disposeDecoders;
  int m_iLastKeyframe;
  double m_dts;
  bool   m_started;
  std::vector<PixelFormat> m_formats;
  double m_decoderPts;
  int    m_skippedDeint;
  bool   m_requestSkipDeint;
  int    m_codecControlFlags;
  CDVDStreamInfo m_hints;
  CDVDCodecOptions m_options;
};
