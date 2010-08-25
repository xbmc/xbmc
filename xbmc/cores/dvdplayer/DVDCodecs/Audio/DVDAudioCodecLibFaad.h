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
#ifdef USE_LIBFAAD_DECODER
#include "DVDAudioCodec.h"
#include "DllLibFaad.h"

#define LIBFAAD_INPUT_SIZE (FAAD_MIN_STREAMSIZE * 8)   // 6144 bytes / 6k
#define LIBFAAD_DECODED_SIZE (16 * LIBFAAD_INPUT_SIZE)

class CDVDAudioCodecLibFaad : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibFaad();
  virtual ~CDVDAudioCodecLibFaad();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()        { return m_iSourceChannels; }
  virtual enum PCMChannels* GetChannelMap();
  virtual int GetSampleRate()      { return m_iSourceSampleRate; }
  virtual int GetBitsPerSample()   { return 16; }
  virtual const char* GetName()    { return "libfaad"; }
  virtual int GetBufferSize()      { return m_InputBufferSize; }

private:

  void CloseDecoder();
  bool OpenDecoder();

  bool SyncStream();

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  enum PCMChannels m_pChannelMap[64];
  int m_iSourceBitrate;

  bool m_bInitializedDecoder;
  bool m_bRawAACStream;

  faacDecHandle m_pHandle;
  faacDecFrameInfo m_frameInfo;

  short* m_DecodedData;
  int   m_DecodedDataSize;

  BYTE m_InputBuffer[LIBFAAD_INPUT_SIZE];
  int  m_InputBufferSize;

  DllLibFaad m_dll;
};

#endif
