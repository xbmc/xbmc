#pragma once

/*
 *      Copyright (C) 2011 Team XBMC
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

#include "system.h"
#include "DVDAudioCodec.h"
#include <AudioFilter/Filter.h>
#include <AudioFilter/SpdifWrapper.h>

class CDVDAudioCodecPassthroughAudioFilter : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthroughAudioFilter();
  virtual ~CDVDAudioCodecPassthroughAudioFilter();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();

  virtual enum PCMChannels *GetChannelMap()
  {
    static enum PCMChannels map[2] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT};
	return map;
  }

  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPassthrough() { return true; }
  virtual const char* GetName()  { return "PassThroughAudioFilter"; }

private:
  int ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize);

  AudioFilter::MultiHeaderParser *m_Mhp;
  AudioFilter::SpdifWrapper m_Spdif;

  BYTE *m_OutputBuffer;
  int m_OutputSize;
  int m_iSourceSampleRate;
};

