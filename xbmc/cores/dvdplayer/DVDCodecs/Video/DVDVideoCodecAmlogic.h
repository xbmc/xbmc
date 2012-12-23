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

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"

class CAMLCodec;

class CDVDVideoCodecAmlogic : public CDVDVideoCodec
{
public:
  CDVDVideoCodecAmlogic();
  virtual ~CDVDVideoCodecAmlogic();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(BYTE *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual int  GetDataSize(void);
  virtual double GetTimeSize(void);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }

protected:
  CAMLCodec       *m_Codec;
  const char      *m_pFormatName;
  DVDVideoPicture m_videobuffer;
  bool            m_opened;
  CDVDStreamInfo  m_hints;
};
