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

#include "threads/Thread.h"
#include <CoreAudio/CoreAudio.h>

#include <list>

typedef std::list<AudioStreamID> AudioStreamIdList;
typedef std::list<AudioStreamRangedDescription> StreamFormatList;

class CCoreAudioStream
{
public:
  CCoreAudioStream();
  virtual ~CCoreAudioStream();
  
  bool    Open(AudioStreamID streamId);
  void    Close();
  
  AudioStreamID GetId() {return m_StreamId;}
  UInt32  GetDirection();
  UInt32  GetTerminalType();
  UInt32  GetNumLatencyFrames();
  bool    GetVirtualFormat(AudioStreamBasicDescription *pDesc);
  bool    GetPhysicalFormat(AudioStreamBasicDescription *pDesc);
  bool    SetVirtualFormat(AudioStreamBasicDescription *pDesc);
  bool    SetPhysicalFormat(AudioStreamBasicDescription *pDesc);
  bool    GetAvailableVirtualFormats(StreamFormatList *pList);
  bool    GetAvailablePhysicalFormats(StreamFormatList *pList);
  
protected:
  static OSStatus HardwareStreamListener(AudioObjectID inObjectID,
    UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[], void* inClientData);

  CEvent m_virtual_format_event;
  CEvent m_physical_format_event;


  AudioStreamID m_StreamId;
  AudioStreamBasicDescription m_OriginalVirtualFormat;  
  AudioStreamBasicDescription m_OriginalPhysicalFormat;  
};

#endif
