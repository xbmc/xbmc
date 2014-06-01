#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "MfcDecoder.h"
#include "FimcConverter.h"

#include "DVDClock.h"
#include "utils/log.h"
#include "guilib/GraphicContext.h"
#include "settings/DisplaySettings.h"
#include <queue>

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

class CDVDVideoCodecMfc : public CDVDVideoCodec
{
public:
  CDVDVideoCodecMfc();
  virtual ~CDVDVideoCodecMfc();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double dts, double pts);
  virtual void Reset();
  bool GetPictureCommon(DVDVideoPicture* pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return m_decoder != NULL ? m_decoder->GetOutputName() : ""; };

private:
  MfcDecoder                   *m_decoder;
  FimcConverter                *m_converter;

  unsigned int                  m_iDecodedWidth;
  unsigned int                  m_iDecodedHeight;
  unsigned int                  m_iConvertedWidth;
  unsigned int                  m_iConvertedHeight;

  std::priority_queue<double>   m_pts;
  std::priority_queue<double>   m_dts;

  bool                          m_NV12Support;
  bool                          m_bDropPictures;
  DVDVideoPicture               m_videoBuffer;
};
