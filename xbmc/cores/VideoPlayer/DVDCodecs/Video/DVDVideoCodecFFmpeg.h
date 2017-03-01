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

#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "DVDVideoCodec.h"
#include "DVDResource.h"
#include "DVDVideoPPFFmpeg.h"
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

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec, public ICallbackHWAccel
{
public:
  CDVDVideoCodecFFmpeg(CProcessInfo &processInfo);
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual void Reopen() override;
  virtual CDVDVideoCodec::VCReturn GetPicture(DVDVideoPicture* pDvdVideoPicture) override;
  virtual const char* GetName() override { return m_name.c_str(); }; // m_name is never changed after open
  virtual unsigned GetConvergeCount() override;
  virtual unsigned GetAllowedReferences() override;
  virtual bool GetCodecStats(double &pts, int &droppedFrames, int &skippedPics) override;
  virtual void SetCodecControl(int flags) override;

  virtual IHardwareDecoder* GetHWAccel() override;
  virtual bool GetPictureCommon(DVDVideoPicture* pDvdVideoPicture) override;

protected:
  void Dispose();
  static enum AVPixelFormat GetFormat(struct AVCodecContext * avctx, const AVPixelFormat * fmt);

  int  FilterOpen(const std::string& filters, bool scale);
  void FilterClose();
  CDVDVideoCodec::VCReturn FilterProcess(AVFrame* frame);
  void SetFilters();
  void UpdateName();
  bool SetPictureParams(DVDVideoPicture* pDvdVideoPicture);

  IHardwareDecoder* CreateVideoDecoderHW(AVPixelFormat pixfmt, CProcessInfo &processInfo);
  bool HasHardware() { return m_pHardware != nullptr; };
  void SetHardware(IHardwareDecoder *hardware);


  AVFrame* m_pFrame;
  AVFrame* m_pDecodedFrame;
  AVCodecContext* m_pCodecContext;

  std::string m_filters;
  std::string m_filters_next;
  AVFilterGraph* m_pFilterGraph;
  AVFilterContext* m_pFilterIn;
  AVFilterContext* m_pFilterOut;
  AVFrame* m_pFilterFrame;
  bool m_filterEof;
  bool m_eof;

  CDVDVideoPPFFmpeg m_postProc;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;
  int m_iOrientation;// orientation of the video in degrees counter clockwise

  unsigned int m_uSurfacesCount;

  std::string m_name;
  int m_decoderState;
  IHardwareDecoder *m_pHardware;
  int m_iLastKeyframe;
  double m_dts;
  bool   m_started;
  std::vector<AVPixelFormat> m_formats;
  double m_decoderPts;
  int    m_skippedDeint;
  int    m_droppedFrames;
  bool   m_requestSkipDeint;
  int    m_codecControlFlags;
  bool m_interlaced;
  double m_DAR;
  CDVDStreamInfo m_hints;
  CDVDCodecOptions m_options;

  struct CDropControl
  {
    CDropControl();
    void Reset(bool init);
    void Process(int64_t pts, bool drop);

    int64_t m_lastPTS;
    int64_t m_diffPTS;
    int m_count;
    enum
    {
      INIT,
      VALID
    } m_state;
  } m_dropCtrl;
};
