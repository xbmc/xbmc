/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(TARGET_DARWIN_OSX)

#include "threads/Event.h"
#include <CoreAudio/CoreAudio.h>
#include <IOKit/audio/IOAudioTypes.h>

#include <list>
#include <vector>


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
  static bool IsDigitalOutput(AudioStreamID id);
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
