/*
 *      Copyright (C) 2005-2013 Team XBMC
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

class CVideoBufferPoolFFmpeg;

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec, public ICallbackHWAccel
{
public:
  explicit CDVDVideoCodecFFmpeg(CProcessInfo &processInfo);
  ~CDVDVideoCodecFFmpeg() override;
  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  bool AddData(const DemuxPacket &packet) override;
  void Reset() override;
  void Reopen() override;
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override { return m_name.c_str(); }; // m_name is never changed after open
  unsigned GetConvergeCount() override;
  unsigned GetAllowedReferences() override;
  bool GetCodecStats(double &pts, int &droppedFrames, int &skippedPics) override;
  void SetCodecControl(int flags) override;

  IHardwareDecoder* GetHWAccel() override;
  bool GetPictureCommon(VideoPicture* pVideoPicture) override;

protected:
  void Dispose();
  static enum AVPixelFormat GetFormat(struct AVCodecContext * avctx, const AVPixelFormat * fmt);

  int  FilterOpen(const std::string& filters, bool scale);
  void FilterClose();
  CDVDVideoCodec::VCReturn FilterProcess(AVFrame* frame);
  void SetFilters();
  void UpdateName();
  bool SetPictureParams(VideoPicture* pVideoPicture);

  bool HasHardware() { return m_pHardware != nullptr; };
  void SetHardware(IHardwareDecoder *hardware);

  AVFrame* m_pFrame = nullptr;;
  AVFrame* m_pDecodedFrame = nullptr;;
  AVCodecContext* m_pCodecContext = nullptr;;
  std::shared_ptr<CVideoBufferPoolFFmpeg> m_videoBufferPool;

  std::string m_filters;
  std::string m_filters_next;
  AVFilterGraph* m_pFilterGraph = nullptr;
  AVFilterContext* m_pFilterIn = nullptr;
  AVFilterContext* m_pFilterOut = nullptr;;
  AVFrame* m_pFilterFrame = nullptr;;
  bool m_filterEof = false;
  bool m_eof = false;

  CDVDVideoPPFFmpeg m_postProc;

  int m_iPictureWidth = 0;
  int m_iPictureHeight = 0;
  int m_iScreenWidth = 0;
  int m_iScreenHeight = 0;
  int m_iOrientation = 0;// orientation of the video in degrees counter clockwise

  std::string m_name;
  int m_decoderState;
  IHardwareDecoder *m_pHardware = nullptr;
  int m_iLastKeyframe = 0;
  double m_dts = DVD_NOPTS_VALUE;
  bool m_started = false;
  bool m_startedInput = false;
  std::vector<AVPixelFormat> m_formats;
  double m_decoderPts = DVD_NOPTS_VALUE;
  int m_skippedDeint = 0;
  int m_droppedFrames = 0;
  bool m_requestSkipDeint = false;
  int m_codecControlFlags = 0;
  bool m_interlaced = false;
  double m_DAR = 1.0;
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
