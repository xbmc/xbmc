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
#include "DllLibMad.h"

#define MAD_INPUT_SIZE (8 * 1024)
#define MAD_DECODED_SIZE (sizeof(float) * MAD_INPUT_SIZE)

class CDVDAudioCodecLibMad : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibMad();
  virtual ~CDVDAudioCodecLibMad();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual CAEChannelInfo GetChannelMap();
  virtual int GetChannels()                 { return m_iSourceChannels;   }
  virtual int GetSampleRate()               { return m_iSourceSampleRate; }
  virtual enum AEDataFormat GetDataFormat() { return AE_FMT_FLOAT;        }
  virtual const char* GetName()             { return "libmad";            }
  virtual int GetBufferSize()               { return m_iInputBufferSize;  }

private:

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  bool m_bInitialized;

  struct mad_synth m_synth;
  struct mad_stream m_stream;
  struct mad_frame m_frame;

  BYTE m_decodedData[MAD_DECODED_SIZE];
  int  m_iDecodedDataSize;

  BYTE m_inputBuffer[MAD_INPUT_SIZE];
  int m_iInputBufferSize;

  DllLibMad m_dll;
};
