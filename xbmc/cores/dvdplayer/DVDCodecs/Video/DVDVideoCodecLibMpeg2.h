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

#include "DVDVideoCodec.h"
#include "DllLibMpeg2.h"

class CDVDVideoCodecLibMpeg2 : public CDVDVideoCodec
{
public:
  CDVDVideoCodecLibMpeg2();
  virtual ~CDVDVideoCodecLibMpeg2();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double dts, double pts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual bool GetUserData(DVDVideoUserData* pDvdVideoUserData);

  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return "libmpeg2"; }

protected:
  DVDVideoPicture* GetBuffer(unsigned int width, unsigned int height);
  inline void ReleaseBuffer(DVDVideoPicture* pPic);
  inline void DeleteBuffer(DVDVideoPicture* pPic);

  int GuessAspect(const mpeg2_sequence_t *sequence, unsigned int *pixel_width, unsigned int *pixel_height);

  mpeg2dec_t* m_pHandle;
  const mpeg2_info_t* m_pInfo;
  DllLibMpeg2 m_dll;

  unsigned int m_irffpattern;
  bool m_bFilm; //Signals that we have film material
  bool m_bIs422;

  int m_hurry;
  double m_dts;
  double m_dts2;
  //The buffer of pictures we need
  DVDVideoPicture m_pVideoBuffer[3];
  DVDVideoPicture* m_pCurrentBuffer;
};
