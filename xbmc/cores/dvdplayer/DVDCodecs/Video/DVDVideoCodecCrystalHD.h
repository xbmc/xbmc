/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#pragma once

#if defined(HAVE_LIBCRYSTALHD)

#include "CrystalHD.h"
#include "DVDVideoCodec.h"

typedef struct H264BSFContext {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t *sps_pps_data;
    uint32_t size;
} H264BSFContext;

class CDVDVideoCodecCrystalHD : public CDVDVideoCodec
{
public:
  CDVDVideoCodecCrystalHD();
  virtual ~CDVDVideoCodecCrystalHD();

  // Required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose(void);
  virtual int  Decode(BYTE *pData, int iSize, double pts);
  virtual void Reset(void);
  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName(void) { return (const char*)m_pFormatName; }

protected:
  bool init_h264_mp4toannexb_filter(CDVDStreamInfo &hints);
  void alloc_and_copy(uint8_t **poutbuf,     int *poutbuf_size,
                const uint8_t *sps_pps, uint32_t sps_pps_size,
                const uint8_t *in,      uint32_t in_size);
  bool h264_mp4toannexb_filter(BYTE* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);

  bool            m_annexbfiltering;
  uint8_t         *m_sps_pps_data;
  uint32_t        m_sps_pps_size;
  H264BSFContext  m_sps_pps_context;

  CCrystalHD      *m_Device;
  int             m_DecodeReturn;
  bool            m_DropPictures;
  const char      *m_pFormatName;
};

#endif
