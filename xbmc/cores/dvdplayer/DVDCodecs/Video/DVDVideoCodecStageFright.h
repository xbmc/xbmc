#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#if defined(HAS_LIBSTAGEFRIGHT)

#include "DVDVideoCodec.h"
#include "utils/BitstreamConverter.h"

class CStageFrightVideo;
class CDVDVideoCodecStageFright : public CDVDVideoCodec
{
public:
  CDVDVideoCodecStageFright();
  virtual ~CDVDVideoCodecStageFright();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  virtual void Reset(void);
  virtual unsigned GetAllowedReferences() { return 2; }
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }
  virtual void SetSpeed(int iSpeed);
  virtual int GetDataSize(void);
  virtual double GetTimeSize(void);

protected:
  const char        *m_pFormatName;
  CStageFrightVideo     *m_stf_decoder;

  bool              m_convert_bitstream;
  CBitstreamConverter   *m_converter;
};

#endif
