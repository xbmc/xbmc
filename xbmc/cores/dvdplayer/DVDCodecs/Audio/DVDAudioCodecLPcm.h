#pragma once

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

#ifndef AUDIO_DVDAUDIOCODEC_H_INCLUDED
#define AUDIO_DVDAUDIOCODEC_H_INCLUDED
#include "DVDAudioCodec.h"
#endif

#ifndef AUDIO_DVDCODECS_DVDCODECS_H_INCLUDED
#define AUDIO_DVDCODECS_DVDCODECS_H_INCLUDED
#include "DVDCodecs/DVDCodecs.h"
#endif

#ifndef AUDIO_DVDAUDIOCODECPCM_H_INCLUDED
#define AUDIO_DVDAUDIOCODECPCM_H_INCLUDED
#include "DVDAudioCodecPcm.h"
#endif


class CDVDAudioCodecLPcm : public CDVDAudioCodecPcm
{
public:
  CDVDAudioCodecLPcm();
  virtual ~CDVDAudioCodecLPcm();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual int Decode(uint8_t* pData, int iSize);
  virtual const char* GetName()  { return "lpcm"; }

protected:

  int m_bufferSize;
  uint8_t *m_buffer;

  AVCodecID m_codecID;

private:
  CDVDAudioCodecLPcm(const CDVDAudioCodecLPcm&);
  CDVDAudioCodecLPcm const& operator=(CDVDAudioCodecLPcm const&);
};
