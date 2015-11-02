/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "RKCodec.h"
#include "utils/BitstreamConverter.h"

#define MEDIA_AUDIO_DEFAULT     0
#define MEDIA_AUDIO_HDMI_BYPASS 6
#define MEDIA_AUDIO_SPDIF      8

class CDVDVideoCodecRK : public CDVDVideoCodec
{

public:
  CDVDVideoCodecRK();
  
  virtual ~CDVDVideoCodecRK();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options); // call from CDVDFactoryCodec :: CreateVideoCodec

  virtual void Dispose(void);

  virtual int  Decode(uint8_t *pData, int iSize, double dts, double pts);

  virtual void Reset(void);

  virtual void Flush(void);

  virtual bool GetPicture(DVDVideoPicture *pDvdVideoPicture);

  virtual void SetSpeed(int iSpeed);

  virtual void SetDropState(bool bDrop);

  virtual int  GetDataSize(void);

  virtual double GetTimeSize(void);

  virtual const char* GetName(void) { return (const char*)m_pFormatName; }

protected:

  bool       isRkHwSupport(CDVDStreamInfo &hints);

  CRKCodec            *m_Codec;
  const char          *m_pFormatName;
  DVDVideoPicture      m_videobuffer;
  CDVDStreamInfo       m_hints;
  bool                 m_opened;
  CBitstreamParser    *m_bitparser;
  CBitstreamConverter *m_bitstream;
  CRect                m_display_rect;

};

void rk_set_audio_passthrough(bool passthrough);
int  rk_get_audio_setting();
int64_t rk_get_adjust_latency();

