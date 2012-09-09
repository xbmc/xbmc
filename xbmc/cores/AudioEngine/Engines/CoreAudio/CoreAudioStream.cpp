/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "CoreAudioStream.h"

#include "CoreAudioAEHAL.h"
#include "utils/log.h"
#include "utils/StdString.h"

CCoreAudioStream::CCoreAudioStream() :
  m_StreamId  (0    )
{
  m_OriginalVirtualFormat.mFormatID = 0;
  m_OriginalPhysicalFormat.mFormatID = 0;
}

CCoreAudioStream::~CCoreAudioStream()
{
  Close();
}

bool CCoreAudioStream::Open(AudioStreamID streamId)
{
  m_StreamId = streamId;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Open: Opened stream 0x%04x.", (uint)m_StreamId);

  // watch for physical property changes.
  AudioObjectPropertyAddress propertyAOPA;
  propertyAOPA.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement  = kAudioObjectPropertyElementMaster;  
  propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
  if (AudioObjectAddPropertyListener(m_StreamId, &propertyAOPA, HardwareStreamListener, this) != noErr)
    CLog::Log(LOGERROR, "CCoreAudioStream::Open: couldn't set up a physical property listener.");

  // watch for virtual property changes.
  propertyAOPA.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement  = kAudioObjectPropertyElementMaster;  
  propertyAOPA.mSelector = kAudioStreamPropertyVirtualFormat;
  if (AudioObjectAddPropertyListener(m_StreamId, &propertyAOPA, HardwareStreamListener, this) != noErr)
    CLog::Log(LOGERROR, "CCoreAudioStream::Open: couldn't set up a virtual property listener.");

  return true;
}

// TODO: Should it even be possible to change both the 
// physical and virtual formats, since the devices do it themselves?
void CCoreAudioStream::Close()
{
  if (!m_StreamId)
    return;

  std::string formatString;

  // remove the physical/virtual property listeners before we make changes
  // that will trigger callbacks that we do not care about.
  AudioObjectPropertyAddress propertyAOPA;
  propertyAOPA.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement  = kAudioObjectPropertyElementMaster;  
  propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
  if (AudioObjectRemovePropertyListener(m_StreamId, &propertyAOPA, HardwareStreamListener, this) != noErr)
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Couldn't remove property listener.");

  propertyAOPA.mScope    = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement  = kAudioObjectPropertyElementMaster;  
  propertyAOPA.mSelector = kAudioStreamPropertyVirtualFormat;
  if (AudioObjectRemovePropertyListener(m_StreamId, &propertyAOPA, HardwareStreamListener, this) != noErr)
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Couldn't remove property listener.");

  // Revert any format changes we made
  if (m_OriginalVirtualFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: "
      "Restoring original virtual format for stream 0x%04x. (%s)",
      (uint)m_StreamId, StreamDescriptionToString(m_OriginalVirtualFormat, formatString));
    AudioStreamBasicDescription setFormat = m_OriginalVirtualFormat;
    SetVirtualFormat(&setFormat);
  }
  if (m_OriginalPhysicalFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: "
      "Restoring original physical format for stream 0x%04x. (%s)",
      (uint)m_StreamId, StreamDescriptionToString(m_OriginalPhysicalFormat, formatString));
    AudioStreamBasicDescription setFormat = m_OriginalPhysicalFormat;
    SetPhysicalFormat(&setFormat);
  }

  m_OriginalVirtualFormat.mFormatID  = 0;
  m_OriginalPhysicalFormat.mFormatID = 0;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Closed stream 0x%04x.", (uint)m_StreamId);
  m_StreamId = 0;
}

UInt32 CCoreAudioStream::GetDirection()
{
  if (!m_StreamId)
    return 0;

  UInt32 val = 0;
  UInt32 size = sizeof(UInt32);

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyDirection; 

  OSStatus ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &size, &val); 
  if (ret)
    return 0;

  return val;
}

UInt32 CCoreAudioStream::GetTerminalType()
{
  if (!m_StreamId)
    return 0;

  UInt32 val = 0;
  UInt32 size = sizeof(UInt32);

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyTerminalType; 

  OSStatus ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &size, &val); 
  if (ret)
    return 0;
  return val;
}

UInt32 CCoreAudioStream::GetNumLatencyFrames()
{
  if (!m_StreamId)
    return 0;

  UInt32 i_param_size = sizeof(uint32_t);
  UInt32 i_param, num_latency_frames = 0;

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyLatency; 

  // number of frames of latency in the AudioStream
  OSStatus ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &i_param_size, &i_param); 
  if (ret == noErr)
  {
    num_latency_frames += i_param;
  }

  return num_latency_frames;
}

bool CCoreAudioStream::GetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;

  UInt32 size = sizeof(AudioStreamBasicDescription);

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyVirtualFormat; 
  OSStatus ret = AudioObjectGetPropertyDataSize(m_StreamId, &propertyAddress, 0, NULL, &size); 
  if (ret)
    return false;

  ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &size, pDesc); 
  if (ret)
    return false;
  return true;
}

bool CCoreAudioStream::SetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;

  std::string formatString;

  if (!m_OriginalVirtualFormat.mFormatID)
  {
    // Store the original format (as we found it) so that it can be restored later
    if (!GetVirtualFormat(&m_OriginalVirtualFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: "
        "Unable to retrieve current virtual format for stream 0x%04x.", (uint)m_StreamId);
      return false;
    }
  }
  m_virtual_format_event.Reset();

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyVirtualFormat; 

  UInt32 propertySize = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioObjectSetPropertyData(m_StreamId, &propertyAddress, 0, NULL, propertySize, pDesc); 
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: "
      "Unable to set virtual format for stream 0x%04x. Error = %s",
      (uint)m_StreamId, GetError(ret).c_str());
    return false;
  }

  // The AudioStreamSetProperty is not only asynchronious,
  // it is also not Atomic, in its behaviour.
  // Therefore we check 5 times before we really give up.
  // FIXME: failing isn't actually implemented yet.
  for (int i = 0; i < 10; ++i)
  {
    AudioStreamBasicDescription checkVirtualFormat;
    if (!GetVirtualFormat(&checkVirtualFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: "
        "Unable to retrieve current physical format for stream 0x%04x.", (uint)m_StreamId);
      return false;
    }
    if (checkVirtualFormat.mSampleRate == pDesc->mSampleRate &&
        checkVirtualFormat.mFormatID == pDesc->mFormatID &&
        checkVirtualFormat.mFramesPerPacket == pDesc->mFramesPerPacket)
    {
      // The right format is now active.
      CLog::Log(LOGDEBUG, "CCoreAudioStream::SetVirtualFormat: "
        "Virtual format for stream 0x%04x. now active (%s)",
        (uint)m_StreamId, StreamDescriptionToString(checkVirtualFormat, formatString));
      break;
    }
    m_virtual_format_event.WaitMSec(100);
  }
  return true;
}

bool CCoreAudioStream::GetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;

  UInt32 size = sizeof(AudioStreamBasicDescription);

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyPhysicalFormat; 

  OSStatus ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &size, pDesc); 
  if (ret)
    return false;
  return true;
}

bool CCoreAudioStream::SetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;

  std::string formatString;

  if (!m_OriginalPhysicalFormat.mFormatID)
  {
    // Store the original format (as we found it) so that it can be restored later
    if (!GetPhysicalFormat(&m_OriginalPhysicalFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: "
        "Unable to retrieve current physical format for stream 0x%04x.", (uint)m_StreamId);
      return false;
    }
  }
  m_physical_format_event.Reset();

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyPhysicalFormat; 

  UInt32 propertySize = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioObjectSetPropertyData(m_StreamId, &propertyAddress, 0, NULL, propertySize, pDesc); 
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: "
      "Unable to set physical format for stream 0x%04x. Error = %s",
      (uint)m_StreamId, GetError(ret).c_str());
    return false;
  }

  // The AudioStreamSetProperty is not only asynchronious,
  // it is also not Atomic, in its behaviour.
  // Therefore we check 5 times before we really give up.
  // FIXME: failing isn't actually implemented yet.
  for(int i = 0; i < 10; ++i)
  {
    AudioStreamBasicDescription checkPhysicalFormat;
    if (!GetPhysicalFormat(&checkPhysicalFormat))
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: "
        "Unable to retrieve current physical format for stream 0x%04x.", (uint)m_StreamId);
      return false;
    }
    if (checkPhysicalFormat.mSampleRate == pDesc->mSampleRate &&
        checkPhysicalFormat.mFormatID   == pDesc->mFormatID   &&
        checkPhysicalFormat.mFramesPerPacket == pDesc->mFramesPerPacket)
    {
      // The right format is now active.
      CLog::Log(LOGDEBUG, "CCoreAudioStream::SetPhysicalFormat: "
        "Physical format for stream 0x%04x. now active (%s)",
        (uint)m_StreamId, StreamDescriptionToString(checkPhysicalFormat, formatString));
      break;
    }
    m_physical_format_event.WaitMSec(100);
  }

  return true;
}

bool CCoreAudioStream::GetAvailableVirtualFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyAvailableVirtualFormats; 

  UInt32 propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_StreamId, &propertyAddress, 0, NULL, &propertySize); 
  if (ret)
    return false;

  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription *pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &propertySize, pFormatList); 
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

bool CCoreAudioStream::GetAvailablePhysicalFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;

  AudioObjectPropertyAddress propertyAddress; 
  propertyAddress.mScope    = kAudioObjectPropertyScopeGlobal; 
  propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
  propertyAddress.mSelector = kAudioStreamPropertyAvailablePhysicalFormats; 

  UInt32 propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_StreamId, &propertyAddress, 0, NULL, &propertySize); 
  if (ret)
    return false;

  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription *pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioObjectGetPropertyData(m_StreamId, &propertyAddress, 0, NULL, &propertySize, pFormatList); 
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

OSStatus CCoreAudioStream::HardwareStreamListener(AudioObjectID inObjectID,
  UInt32 inNumberAddresses, const AudioObjectPropertyAddress inAddresses[], void *inClientData)
{
  CCoreAudioStream *ca_stream = (CCoreAudioStream*)inClientData;

  for (UInt32 i = 0; i < inNumberAddresses; i++)
  {
    if (inAddresses[i].mSelector == kAudioStreamPropertyPhysicalFormat)
    {
      AudioStreamBasicDescription actualFormat;
      UInt32 propertySize = sizeof(AudioStreamBasicDescription);
      // hardware physical format has changed.
      if (AudioObjectGetPropertyData(ca_stream->m_StreamId, &inAddresses[i], 0, NULL, &propertySize, &actualFormat) == noErr)
      {
        CStdString formatString;
        CLog::Log(LOGINFO, "CCoreAudioStream::HardwareStreamListener: "
          "Hardware physical format changed to %s", StreamDescriptionToString(actualFormat, formatString));
        ca_stream->m_physical_format_event.Set();
      }
    }
    else if (inAddresses[i].mSelector == kAudioStreamPropertyVirtualFormat)
    {
      // hardware virtual format has changed.
      AudioStreamBasicDescription actualFormat;
      UInt32 propertySize = sizeof(AudioStreamBasicDescription);
      if (AudioObjectGetPropertyData(ca_stream->m_StreamId, &inAddresses[i], 0, NULL, &propertySize, &actualFormat) == noErr)
      {
        CStdString formatString;
        CLog::Log(LOGINFO, "CCoreAudioStream::HardwareStreamListener: "
          "Hardware virtual format changed to %s", StreamDescriptionToString(actualFormat, formatString));
        ca_stream->m_virtual_format_event.Set();
      }
    }
  }

  return noErr;
}
