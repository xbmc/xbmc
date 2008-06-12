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

#include "CachingCodec.h"
#include "DllAc3codec.h"

#ifdef HAS_AC3_CODEC

class AC3Codec : public CachingCodec
{
public:
  AC3Codec();
  virtual ~AC3Codec();

  virtual bool    Init(const CStdString &strFile, unsigned int filecache);
  virtual void    DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int     ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool    CanInit();

protected:
  virtual bool CalculateTotalTime();
  virtual int  ReadInput();
  virtual void PrepairBuffers();
  virtual bool InitFile(const CStdString &strFile, unsigned int filecache);
  virtual void CloseFile();
  virtual void SetDefault();
  
  int  Decode(BYTE* pData, int iSize);
  void SetupChannels(unsigned flags);
  
  a52_state_t* m_pState;

  BYTE m_inputBuffer[3840];
  BYTE* m_pInputBuffer;

  BYTE* m_readBuffer;
  unsigned int m_readingBufferSize;
  unsigned int m_readBufferPos;

  BYTE* m_decodedData;
  unsigned int m_decodingBufferSize;
  unsigned int m_decodedDataSize;

  bool m_eof;
  int  m_iDataStart;
  bool m_IsInitialized;
  bool m_DecoderError;

  int    m_iFrameSize;
  int    m_iFlags;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;
  unsigned int m_iOutputMapping;

  DllAc3Codec m_dll;
};
#endif
