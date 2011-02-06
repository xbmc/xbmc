#pragma once

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

#include "DVDAudioCodec.h"
#include "DllAvCodec.h"
#include "DllAvCore.h"
#include "DllAvFormat.h"
#include "DllAvUtil.h"

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
  virtual AEChLayout GetChannelMap();
  virtual int GetSampleRate();
  virtual enum AEDataFormat GetDataFormat();
  virtual const char* GetName() { return "FFmpeg"; }
  virtual int GetBufferSize() { return m_iBuffered; }

protected:
  AVCodecContext*   m_pCodecContext;
  AEChannel         m_channelLayout[AE_CH_MAX];
  int               m_iMapChannels;

  BYTE *m_pBuffer1;
  int   m_iBufferSize1;

  bool m_bOpenedCodec;
  int m_iBuffered;

  int     m_channels;
  int64_t m_layout;

  DllAvCodec m_dllAvCodec;
  DllAvCore m_dllAvCore;
  DllAvUtil m_dllAvUtil;

  void BuildChannelMap();
};

