#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "cores/AudioEngine/Utils/AEAudioFormat.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

#include "DVDStreamInfo.h"
#include "platform/linux/PlatformDefs.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

class COMXAudioCodecOMX
{
public:
  explicit COMXAudioCodecOMX(CProcessInfo &processInfo);
  virtual ~COMXAudioCodecOMX();
  bool Open(CDVDStreamInfo &hints);
  void Dispose();
  int Decode(unsigned char* pData, int iSize, double dts, double pts);
  int GetData(unsigned char** dst, double &dts, double &pts);
  void Reset();
  int GetChannels();
  void BuildChannelMap();
  CAEChannelInfo GetChannelMap();
  int GetSampleRate();
  int GetBitsPerSample();
  static const char* GetName() { return "FFmpeg"; }
  int GetBitRate();
  unsigned int GetFrameSize() { return m_frameSize; }

protected:
  CProcessInfo &m_processInfo;
  AVCodecContext* m_pCodecContext;
  SwrContext*     m_pConvert;
  enum AVSampleFormat m_iSampleFormat;
  enum AVSampleFormat m_desiredSampleFormat;

  AVFrame* m_pFrame1;

  unsigned char *m_pBufferOutput;
  int   m_iBufferOutputUsed;
  int   m_iBufferOutputAlloced;

  int     m_channels;
  CAEChannelInfo m_channelLayout;
  bool m_bFirstFrame;
  bool m_bGotFrame;
  bool m_bNoConcatenate;
  unsigned int  m_frameSize;
  double m_dts, m_pts;
};
