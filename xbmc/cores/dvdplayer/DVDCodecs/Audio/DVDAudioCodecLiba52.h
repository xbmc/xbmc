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

#include "system.h"
#ifdef USE_LIBA52_DECODER

#include "DVDAudioCodec.h"
#include "DllLiba52.h"

#ifdef _LINUX
#define XBMC_ALIGN_INT __attribute__ ((aligned (4)))
#else
#define XBMC_ALIGN_INT
#endif

class CDVDAudioCodecLiba52 : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLiba52();
  virtual ~CDVDAudioCodecLiba52();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()        { return m_iOutputChannels; }
  virtual enum PCMChannels *GetChannelMap() { return m_pChannelMap; }
  virtual int GetSampleRate()      { return m_iSourceSampleRate; }
  virtual int GetBufferSize()      { return m_inputSize; }
  virtual int GetBitsPerSample()   { return 16; }
  virtual const char* GetName()    { return "liba52"; }

protected:
  void SetDefault();
  void SetupChannels(int flags);

  int ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize);

  a52_state_t* m_pState;

  int m_iFrameSize;
  float* m_fSamples;

  bool m_bFlagsInitialized;
  int m_iSourceSampleRate;
  int m_iSourceFlags;
  int m_iSourceBitrate;
  int m_iSourceChannels;

  int m_iOutputFlags;
  int m_iOutputChannels;
  enum PCMChannels *m_pChannelMap;

  DllLiba52 m_dll;

  float m_Gain;

  BYTE m_decodedData[131072] XBMC_ALIGN_INT; // could be a bit to big
  int  m_decodedSize;

  BYTE m_inputBuffer[4096] XBMC_ALIGN_INT;
  int  m_inputSize;
};
#endif
