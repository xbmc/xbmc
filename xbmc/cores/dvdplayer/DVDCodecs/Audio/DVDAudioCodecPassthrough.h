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
#if (defined(USE_LIBA52_DECODER) || defined(USE_LIBDTS_DECODER)) && !defined(WIN32)

#include "DVDAudioCodec.h"
#ifdef USE_LIBA52_DECODER
  #include "DllLiba52.h"
#endif
#ifdef USE_LIBDTS_DECODER
  #include "DllLibDts.h"
#endif

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough();
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual enum PCMChannels *GetChannelMap() { static enum PCMChannels map[2] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT}; return map; }
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPassthrough() { return true; }
  virtual const char* GetName()  { return "passthrough"; }

private:
  int ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize);

#ifdef USE_LIBA52_DECODER
  DllLiba52    m_dllA52;
  a52_state_t* m_pStateA52;
  int PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut);
#endif

#ifdef USE_LIBDTS_DECODER
  DllLibDts    m_dllDTS;
  dts_state_t* m_pStateDTS;
  int PaddDTSData( BYTE* pData, int iDataSize, BYTE* pOut);
#endif

  BYTE *m_OutputBuffer;
  int   m_OutputSize;
  BYTE *m_InputBuffer;
  int   m_InputSize;

  int m_iFrameSize;

  int m_iSamplesPerFrame;
  int m_iSampleRate;

  int     m_Codec;
  bool    m_Synced;

  int m_iSourceFlags;
  int m_iSourceSampleRate;
  int m_iSourceBitrate;
};

#endif
