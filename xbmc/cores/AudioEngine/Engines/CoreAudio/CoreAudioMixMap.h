#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#include "AEAudioFormat.h"

#include <CoreAudio/CoreAudio.h>

class CAUMatrixMixer;
class CAUOutputDevice;

class CCoreAudioMixMap
{
public:
  CCoreAudioMixMap();
  CCoreAudioMixMap(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout);
  virtual ~CCoreAudioMixMap();

  operator Float32*() const {return m_pMap;}

  const Float32*  GetBuffer()         {return m_pMap;}
  UInt32          GetInputChannels()  {return m_inChannels;}
  UInt32          GetOutputChannels() {return m_outChannels;}
  bool            IsValid() {return m_isValid;}
  void            Rebuild(AudioChannelLayout& inLayout, AudioChannelLayout& outLayout);
  static          CCoreAudioMixMap *CreateMixMap(CAUOutputDevice *audioUnit,
                    AEAudioFormat &format, AudioChannelLayoutTag layoutTag);
  static bool     SetMixingMatrix(CAUMatrixMixer *mixerUnit, CCoreAudioMixMap *mixMap,
                    AudioStreamBasicDescription *inputFormat, AudioStreamBasicDescription *fmt, int channelOffset);
private:
  Float32         *m_pMap;
  UInt32          m_inChannels;
  UInt32          m_outChannels;
  static UInt32   m_deviceChannels;
  bool            m_isValid;
};

#endif
