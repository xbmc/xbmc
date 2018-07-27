/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavutil/frame.h"
}

struct SwrContext;

namespace ActiveAE
{

class CActiveAEFilter
{
public:
  CActiveAEFilter();
  virtual ~CActiveAEFilter();
  void Init(AVSampleFormat fmt, int sampleRate, uint64_t channelLayout);
  int ProcessFilter(uint8_t **dst_buffer, int dst_samples, uint8_t **src_buffer, int src_samples, int src_bufsize);
  bool SetTempo(float tempo);
  bool NeedData();
  bool IsEof();
  bool IsActive();
  int GetBufferedSamples();

protected:
  bool CreateFilterGraph();
  bool CreateAtempoFilter();
  void CloseFilter();

  AVSampleFormat m_sampleFormat;
  int m_sampleRate;
  uint64_t m_channelLayout;
  AVFilterGraph* m_pFilterGraph;
  AVFilterContext* m_pFilterCtxIn;
  AVFilterContext* m_pFilterCtxOut;
  AVFilterContext* m_pFilterCtxAtempo;
  AVFrame* m_pOutFrame;
  SwrContext* m_pConvertCtx;
  AVFrame* m_pConvertFrame;
  bool m_needConvert;
  float m_tempo;
  bool m_filterEof;
  bool m_started;
  bool m_hasData;
  bool m_needData;
  int m_sampleOffset;
  int64_t m_SamplesIn;
  int64_t m_SamplesOut;
};

}
