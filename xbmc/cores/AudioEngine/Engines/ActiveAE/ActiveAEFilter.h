#pragma once
/*
 *      Copyright (C) 2010-2016 Team Kodi
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
  bool m_hasData;
  bool m_needData;
  int m_sampleOffset;
  int m_bufferedSamples;
};

}