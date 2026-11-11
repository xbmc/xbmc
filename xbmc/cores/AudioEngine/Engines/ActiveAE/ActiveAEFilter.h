/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
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
  /*!
   * \brief Change the tempo of the running filter, keeping what it has buffered.
   *
   * \param tempo the new tempo
   * \return false if the tempo can't be changed in place and the filter has to be re-created
   *         with SetTempo, true otherwise
   */
  bool ChangeTempo(float tempo);
  bool NeedData() const;
  bool IsEof() const;
  bool IsActive() const;
  int GetBufferedSamples() const;

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
  int m_atempoStages{1};
  AVFrame* m_pOutFrame;
  SwrContext* m_pConvertCtx;
  AVFrame* m_pConvertFrame;
  bool m_needConvert;
  float m_tempo;
  bool m_filterEof;
  bool m_started;
  bool m_hasData;
  bool m_needData;
  bool m_ptsInitialized;
  int m_sampleOffset;
  int64_t m_SamplesIn;
  int64_t m_SamplesOut;
  // Input samples covered by the output before the current frame, each frame at its own tempo
  double m_samplesConsumed{0.0};
  // Tempo the current output frame was produced at
  float m_frameTempo{1.0f};
};

}
