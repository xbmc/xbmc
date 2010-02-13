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

#include "DllWAVPack.h"
#include "CachingCodec.h"

class WAVPackCodec : public CachingCodec
{
public:
  WAVPackCodec();
  virtual ~WAVPackCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  static int ReadCallback(void *id, void *data, int bcount);
  static unsigned int GetPosCallback(void *id);
  static int SetPosAbsCallback(void *id, unsigned int pos);
  static int SetPosRelCallback(void *id, int delta, int mode);
  static unsigned int GetLengthCallback(void *id);
  static int CanSeekCallback(void *id);
  static int PushBackByteCallback(void *id, int c);

  void FormatSamples (BYTE *dst, int bps, long *src, unsigned long samcnt);

  char m_errormsg[512];
  WavpackContext* m_Handle;
  #if (!defined WIN32)
  WavpackStreamReader m_Callbacks;
  #else
  stream_reader m_Callbacks;
  #endif

  BYTE*     m_Buffer;
  int       m_BufferSize;
  int       m_BufferPos;
  BYTE*     m_ReadBuffer;

  DllWavPack m_dll;
};

