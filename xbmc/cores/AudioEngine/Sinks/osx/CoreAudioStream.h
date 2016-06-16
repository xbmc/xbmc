#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#include "threads/Event.h"
#include <CoreAudio/CoreAudio.h>
#include <IOKit/audio/IOAudioTypes.h>

#include <list>
#include <vector>

// not defined in 10.6 sdk
#ifndef kIOAudioDeviceTransportTypeThunderbolt
#define kIOAudioDeviceTransportTypeThunderbolt 'thun'
#endif


typedef std::vector<AudioStreamID> AudioStreamIdList;
typedef std::vector<AudioStreamRangedDescription> StreamFormatList;

class CCoreAudioStream
{
public:
  CCoreAudioStream();
  virtual ~CCoreAudioStream();
  
  bool    Open(AudioStreamID streamId);
  void    Close(bool restore = true);

  AudioStreamID GetId() {return m_StreamId;}
  UInt32  GetDirection();
  static UInt32 GetTerminalType(AudioStreamID id);
  UInt32  GetNumLatencyFrames();
  bool    GetVirtualFormat(AudioStreamBasicDescription *pDesc);
  bool    GetPhysicalFormat(AudioStreamBasicDescription *pDesc);
  bool    SetVirtualFormat(AudioStreamBasicDescription *pDesc);
  bool    SetPhysicalFormat(AudioStreamBasicDescription *pDesc);
  bool    GetAvailableVirtualFormats(StreamFormatList *pList);
  bool    GetAvailablePhysicalFormats(StreamFormatList *pList);
  static bool GetAvailableVirtualFormats(AudioStreamID id, StreamFormatList *pList);
  static bool GetAvailablePhysicalFormats(AudioStreamID id, StreamFormatList *pList);
  static bool IsDigitalOuptut(AudioStreamID id);
  static bool GetStartingChannelInDevice(AudioStreamID id, UInt32 &startingChannel);

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
