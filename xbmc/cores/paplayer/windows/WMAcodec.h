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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../CachingCodec.h"
#include <wmsdk.h>
#include "utils/RingBuffer.h"
#include "win32/XBMCistream.h"

class WMAcodec : public CachingCodec
{
public:
  WMAcodec();
  virtual ~WMAcodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual int ReadSamples(float *pBuffer, int numsamples, int *actualsamples);
  virtual bool CanInit();

private:
  IWMSyncReader* m_ISyncReader;
  INSSBuffer* m_pINSSBuffer;
  CRingBuffer m_pcmBuffer;
  bool m_bnomoresamples;
  DWORD m_uimaxwritebuffer;
  __int64 m_iDataPos;
  XBMCistream* m_pStream;
};