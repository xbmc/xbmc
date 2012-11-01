#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDAudioCodec.h"
#include "DllAvCodec.h"
#include "DllAvFormat.h"
#include "DllAvUtil.h"
#include "DllSwResample.h"

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg();
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual CAEChannelInfo GetChannelMap();
  virtual int GetSampleRate();
  virtual int GetEncodedSampleRate();
  virtual enum AEDataFormat GetDataFormat();
  virtual const char* GetName() { return "FFmpeg"; }
  virtual int GetBufferSize() { return m_iBuffered; }
  virtual int GetBitRate();

protected:
  AVCodecContext*     m_pCodecContext;
  SwrContext*         m_pConvert;
  enum AVSampleFormat m_iSampleFormat;  
  CAEChannelInfo      m_channelLayout;
  bool                m_bLpcmMode;  

  AVFrame* m_pFrame1;
  int      m_iBufferSize1;
  BYTE*    m_pBuffer2;
  int      m_iBufferSize2;

  bool m_bOpenedCodec;
  int m_iBuffered;

  int      m_channels;
  uint64_t m_layout;

  DllAvCodec m_dllAvCodec;
  DllAvUtil m_dllAvUtil;
  DllSwResample m_dllSwResample;

  void BuildChannelMap();
  void ConvertToFloat();  
};

