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

#include "CoreAudioDevice.h"
#include "CoreAudioHelpers.h"
#include "CoreAudioChannelLayout.h"
#include "CoreAudioHardware.h"
#include "utils/log.h"
#include "osx/DarwinUtils.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioDevice::CCoreAudioDevice()  :
  m_Started             (false    ),
  m_DeviceId            (0        ),
  m_MixerRestore        (-1       ),
  m_IoProc              (NULL     ),
  m_ObjectListenerProc  (NULL     ),
  m_SampleRateRestore   (0.0f     ),
  m_HogPid              (-1       ),
  m_frameSize           (0        ),
  m_OutputBufferIndex   (0        ),
  m_BufferSizeRestore   (0        )
{
}

CCoreAudioDevice::CCoreAudioDevice(AudioDeviceID deviceId) :
  m_Started             (false    ),
  m_DeviceId            (deviceId ),
  m_MixerRestore        (-1       ),
  m_IoProc              (NULL     ),
  m_ObjectListenerProc  (NULL     ),
  m_SampleRateRestore   (0.0f     ),
  m_HogPid              (-1       ),
  m_frameSize           (0        ),
  m_OutputBufferIndex   (0        ),
  m_BufferSizeRestore   (0        )
{
}

CCoreAudioDevice::~CCoreAudioDevice()
{
  Close();
}

bool CCoreAudioDevice::Open(AudioDeviceID deviceId)
{
  m_DeviceId = deviceId;
  m_BufferSizeRestore = GetBufferSize();
  return true;
}

void CCoreAudioDevice::Close()
{
  if (!m_DeviceId)
    return;

  // Stop the device if it was started
  Stop();

  // Unregister the IOProc if we have one
  RemoveIOProc();

  SetHogStatus(false);
  CCoreAudioHardware::SetAutoHogMode(false);

  if (m_MixerRestore > -1) // We changed the mixer status
    SetMixingSupport((m_MixerRestore ? true : false));
  m_MixerRestore = -1;

  if (m_SampleRateRestore != 0.0f)
    SetNominalSampleRate(m_SampleRateRestore);

  if (m_BufferSizeRestore && m_BufferSizeRestore != GetBufferSize())
  {
    SetBufferSize(m_BufferSizeRestore);
    m_BufferSizeRestore = 0;
  }

  m_IoProc = NULL;
  m_DeviceId = 0;
  m_ObjectListenerProc = NULL;
}

void CCoreAudioDevice::Start()
{
  if (!m_DeviceId || m_Started)
    return;

  OSStatus ret = AudioDeviceStart(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Start: "
      "Unable to start device. Error = %s", GetError(ret).c_str());
  else
    m_Started = true;
}

void CCoreAudioDevice::Stop()
{
  if (!m_DeviceId || !m_Started)
    return;

  OSStatus ret = AudioDeviceStop(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: "
      "Unable to stop device. Error = %s", GetError(ret).c_str());
  m_Started = false;
}

void CCoreAudioDevice::RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData)
{
  if (!m_DeviceId)
    return;

  AudioObjectPropertyAddress audioProperty;
  audioProperty.mSelector = kAudioObjectPropertySelectorWildcard;
  audioProperty.mScope = kAudioObjectPropertyScopeWildcard;
  audioProperty.mElement = kAudioObjectPropertyElementWildcard;

  OSStatus ret = AudioObjectRemovePropertyListener(m_DeviceId, &audioProperty, callback, pClientData);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveObjectListenerProc: "
      "Unable to set ObjectListener callback. Error = %s", GetError(ret).c_str());
  }
  m_ObjectListenerProc = NULL;
}

bool CCoreAudioDevice::SetObjectListenerProc(AudioObjectPropertyListenerProc callback, void* pClientData)
{
  // Allow only one ObjectListener at a time
  if (!m_DeviceId || m_ObjectListenerProc)
    return false;

  AudioObjectPropertyAddress audioProperty;
  audioProperty.mSelector = kAudioObjectPropertySelectorWildcard;
  audioProperty.mScope = kAudioObjectPropertyScopeWildcard;
  audioProperty.mElement = kAudioObjectPropertyElementWildcard;

  OSStatus ret = AudioObjectAddPropertyListener(m_DeviceId, &audioProperty, callback, pClientData);

  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetObjectListenerProc: "
      "Unable to remove ObjectListener callback. Error = %s", GetError(ret).c_str());
    return false;
  }

  m_ObjectListenerProc = callback;
  return true;
}
bool CCoreAudioDevice::AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData)
{
  // Allow only one IOProc at a time
  if (!m_DeviceId || m_IoProc)
    return false;

  OSStatus ret = AudioDeviceCreateIOProcID(m_DeviceId, ioProc, pCallbackData, &m_IoProc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::AddIOProc: "
      "Unable to add IOProc. Error = %s", GetError(ret).c_str());
    m_IoProc = NULL;
    return false;
  }

  Start();

  return true;
}

bool CCoreAudioDevice::RemoveIOProc()
{
  if (!m_DeviceId || !m_IoProc)
    return false;

  Stop();

  OSStatus ret = AudioDeviceDestroyIOProcID(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveIOProc: "
      "Unable to remove IOProc. Error = %s", GetError(ret).c_str());

  m_IoProc = NULL; // Clear the reference no matter what

  Sleep(100);

  return true;
}

std::string CCoreAudioDevice::GetName() const
{
  if (!m_DeviceId)
    return "";

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDeviceName;

  UInt32 propertySize;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &propertySize);
  if (ret != noErr)
    return "";

  std::string name;
  char *buff = new char[propertySize + 1];
  buff[propertySize] = 0x00;
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, buff);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetName: "
      "Unable to get device name - id: 0x%04x. Error = %s", (uint)m_DeviceId, GetError(ret).c_str());
  }
  else
  {
    name = buff;
    name.erase(name.find_last_not_of(" ") + 1);
  }
  delete[] buff;

  return name;
}

bool CCoreAudioDevice::IsDigital() const
{
  bool isDigital = false;
  UInt32 transportType = 0;
  if (!m_DeviceId)
    return false;
    
  transportType = GetTransportType();
  if (transportType == INT_MAX)
    return false;

  if (transportType == kIOAudioDeviceTransportTypeFireWire)
    isDigital = true;
  if (transportType == kIOAudioDeviceTransportTypeUSB)
    isDigital = true;
  if (transportType == kIOAudioDeviceTransportTypeHdmi)
    isDigital = true;
  if (transportType == kIOAudioDeviceTransportTypeDisplayPort)
    isDigital = true;
  if (transportType == kIOAudioDeviceTransportTypeThunderbolt)
    isDigital = true;
  if (transportType == kAudioStreamTerminalTypeDigitalAudioInterface)
    isDigital = true;
    
  return isDigital;
}

UInt32 CCoreAudioDevice::GetTransportType() const
{
  UInt32 transportType = 0;
  if (!m_DeviceId)
    return INT_MAX;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyTransportType;

  UInt32 propertySize = sizeof(transportType);
  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &transportType);
  if (ret != noErr)
      return INT_MAX;
  return transportType;
}

UInt32 CCoreAudioDevice::GetTotalOutputChannels() const
{
  UInt32 channels = 0;

  if (!m_DeviceId)
    return channels;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;

  UInt32 size = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &size);
  if (ret != noErr)
    return channels;

  AudioBufferList* pList = (AudioBufferList*)malloc(size);
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &size, pList);
  if (ret == noErr)
  {
    for(UInt32 buffer = 0; buffer < pList->mNumberBuffers; ++buffer)
      channels += pList->mBuffers[buffer].mNumberChannels;
  }
  else
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetTotalOutputChannels: "
      "Unable to get total device output channels - id: 0x%04x. Error = %s",
      (uint)m_DeviceId, GetError(ret).c_str());
  }

  free(pList);

  return channels;
}

UInt32 CCoreAudioDevice::GetNumChannelsOfStream(UInt32 streamIdx) const
{
  UInt32 channels = 0;
  
  if (!m_DeviceId)
    return channels;
  
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
  
  UInt32 size = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &size);
  if (ret != noErr)
    return channels;
  
  AudioBufferList* pList = (AudioBufferList*)malloc(size);
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &size, pList);
  if (ret == noErr)
  {
    if (streamIdx < pList->mNumberBuffers)
      channels = pList->mBuffers[streamIdx].mNumberChannels;
  }
  else
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetNumChannelsOfStream: "
              "Unable to get number of stream output channels - id: 0x%04x. Error = %s",
              (uint)m_DeviceId, GetError(ret).c_str());
  }
  
  free(pList);
  
  return channels;
}

bool CCoreAudioDevice::GetStreams(AudioStreamIdList* pList)
{
  if (!pList || !m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyStreams;

  UInt32  propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &propertySize);
  if (ret != noErr)
    return false;

  UInt32 streamCount = propertySize / sizeof(AudioStreamID);
  AudioStreamID* pStreamList = new AudioStreamID[streamCount];
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, pStreamList);
  if (ret == noErr)
  {
    for (UInt32 stream = 0; stream < streamCount; stream++)
      pList->push_back(pStreamList[stream]);
  }
  delete[] pStreamList;

  return ret == noErr;
}


bool CCoreAudioDevice::IsRunning()
{
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDeviceIsRunning;

  UInt32 isRunning = 0;
  UInt32 propertySize = sizeof(isRunning);
  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &isRunning);
  if (ret != noErr)
    return false;

  return isRunning != 0;
}

bool CCoreAudioDevice::SetHogStatus(bool hog)
{
  // According to Jeff Moore (Core Audio, Apple), Setting kAudioDevicePropertyHogMode
  // is a toggle and the only way to tell if you do get hog mode is to compare
  // the returned pid against getpid, if the match, you have hog mode, if not you don't.
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyHogMode;

  if (hog)
  {
    // Not already set
    if (m_HogPid == -1)
    {
      OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, sizeof(m_HogPid), &m_HogPid);

      // even if setting hogmode was successfull our PID might not get written
      // into m_HogPid (so it stays -1). Readback hogstatus for judging if we
      // had success on getting hog status
      // We do this only when AudioObjectSetPropertyData didn't set m_HogPid because
      // it seems that in the other cases the GetHogStatus could return -1
      // which would overwrite our valid m_HogPid again
      // Man we should never touch this shit again ;)
      if (m_HogPid == -1)
        m_HogPid = GetHogStatus();

      if (ret || m_HogPid != getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: "
          "Unable to set 'hog' status. Error = %s", GetError(ret).c_str());
        return false;
      }
    }
  }
  else
  {
    // Currently Set
    if (m_HogPid > -1)
    {
      pid_t hogPid = -1;
      OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, sizeof(hogPid), &hogPid);
      if (ret || hogPid == getpid())
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: "
          "Unable to release 'hog' status. Error = %s", GetError(ret).c_str());
        return false;
      }
      // Reset internal state
      m_HogPid = hogPid;
    }
  }
  return true;
}

pid_t CCoreAudioDevice::GetHogStatus()
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyHogMode;

  pid_t hogPid = -1;
  UInt32 size = sizeof(hogPid);
  AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &size, &hogPid);

  return hogPid;
}

bool CCoreAudioDevice::SetMixingSupport(UInt32 mix)
{
  if (!m_DeviceId)
    return false;

  if (!GetMixingSupport())
    return false;

  int restore = -1;
  if (m_MixerRestore == -1)
  {
    // This is our first change to this setting. Store the original setting for restore
    restore = (GetMixingSupport() ? 1 : 0);
  }

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertySupportsMixing;

  UInt32 mixEnable = mix ? 1 : 0;
  OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, sizeof(mixEnable), &mixEnable);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: "
      "Unable to set MixingSupport to %s. Error = %s", mix ? "'On'" : "'Off'", GetError(ret).c_str());
    return false;
  }
  if (m_MixerRestore == -1)
    m_MixerRestore = restore;
  return true;
}

bool CCoreAudioDevice::GetMixingSupport()
{
  if (!m_DeviceId)
    return false;

  UInt32    size;
  UInt32    mix = 0;
  Boolean   writable = false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertySupportsMixing;

  if( AudioObjectHasProperty( m_DeviceId, &propertyAddress ) )
  {
    OSStatus ret = AudioObjectIsPropertySettable(m_DeviceId, &propertyAddress, &writable);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioDevice::SupportsMixing: "
        "Unable to get propertyinfo mixing support. Error = %s", GetError(ret).c_str());
      writable = false;
    }

    if (writable)
    {
      size = sizeof(mix);
      ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &size, &mix);
      if (ret != noErr)
        mix = 0;
    }
  }
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::SupportsMixing: "
    "Device mixing support : %s.", mix ? "'Yes'" : "'No'");

  return (mix > 0);
}

bool CCoreAudioDevice::SetCurrentVolume(Float32 vol)
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kHALOutputParam_Volume;

  OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, sizeof(Float32), &vol);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetCurrentVolume: "
      "Unable to set AudioUnit volume. Error = %s", GetError(ret).c_str());
    return false;
  }
  return true;
}

bool CCoreAudioDevice::GetPreferredChannelLayout(CCoreAudioChannelLayout& layout) const
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;

  UInt32 propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &propertySize);
  if (ret)
    return false;

  void* pBuf = malloc(propertySize);
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, pBuf);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetPreferredChannelLayout: "
      "Unable to retrieve preferred channel layout. Error = %s", GetError(ret).c_str());
  else
  {
    // Copy the result into the caller's instance
    layout.CopyLayout(*((AudioChannelLayout*)pBuf));
  }
  free(pBuf);
  return (ret == noErr);
}

bool CCoreAudioDevice::GetPreferredChannelLayoutForStereo(CCoreAudioChannelLayout &layout) const
{
  if (!m_DeviceId)
    return false;
  
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;

  UInt32 channels[2];// this will receive the channel labels
  UInt32 propertySize = sizeof(channels);

  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &channels);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetPreferredChannelLayoutForStereo: "
              "Unable to retrieve preferred channel layout. Error = %s", GetError(ret).c_str());
  else
  {
    // Copy/generate a layout into the result into the caller's instance
    layout.CopyLayoutForStereo(channels);
  }
  return (ret == noErr);
}

std::string CCoreAudioDevice::GetCurrentDataSourceName() const
{
  UInt32 dataSourceId = 0;
  std::string dataSourceName = "";
  if(GetDataSource(dataSourceId))
  {
    dataSourceName = GetDataSourceName(dataSourceId);
  }
  return dataSourceName;
}

std::string CCoreAudioDevice::GetDataSourceName(UInt32 dataSourceId) const
{
  UInt32 propertySize = 0;
  CFStringRef dataSourceNameCF;
  std::string dataSourceName;
  std::string ret = "";
  
  if (!m_DeviceId)
    return ret;
  
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDataSourceNameForIDCFString;

  AudioValueTranslation translation;
  translation.mInputData = &dataSourceId;
  translation.mInputDataSize = sizeof(UInt32);
  translation.mOutputData = &dataSourceNameCF;
  translation.mOutputDataSize = sizeof ( CFStringRef );
  propertySize = sizeof(AudioValueTranslation);
  OSStatus status = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &translation);

  if (( status == noErr ) && dataSourceNameCF )
  {
    if (CDarwinUtils::CFStringRefToUTF8String(dataSourceNameCF, dataSourceName))
    {
      ret = dataSourceName;
    }
    CFRelease ( dataSourceNameCF );
  }

  return ret;
}

bool CCoreAudioDevice::GetDataSource(UInt32 &dataSourceId) const
{
  bool ret = false;
  
  if (!m_DeviceId)
    return false;
  
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDataSource;
  
  UInt32 size = sizeof(dataSourceId);
  OSStatus status = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &size, &dataSourceId);
  if(status == noErr)
    ret = true;
  
  return ret;
}

bool CCoreAudioDevice::SetDataSource(UInt32 &dataSourceId)
{
  bool ret = false;
  
  if (!m_DeviceId)
    return false;
  
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDataSource;
  
  UInt32 size = sizeof(dataSourceId);
  OSStatus status = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, size, &dataSourceId);
  if(status == noErr)
    ret = true;
  
  return ret;
}

bool CCoreAudioDevice::GetDataSources(CoreAudioDataSourceList* pList) const
{
  if (!pList || !m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyDataSources;

  UInt32 propertySize = 0;
  OSStatus ret = AudioObjectGetPropertyDataSize(m_DeviceId, &propertyAddress, 0, NULL, &propertySize);
  if (ret != noErr)
    return false;

  UInt32  sources = propertySize / sizeof(UInt32);
  UInt32* pSources = new UInt32[sources];
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, pSources);
  if (ret == noErr)
  {
    for (UInt32 i = 0; i < sources; i++)
      pList->push_back(pSources[i]);
  }
  delete[] pSources;
  return (!ret);
}

Float64 CCoreAudioDevice::GetNominalSampleRate()
{
  if (!m_DeviceId)
    return 0.0f;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;

  Float64 sampleRate = 0.0f;
  UInt32  propertySize = sizeof(Float64);
  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &sampleRate);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetNominalSampleRate: "
      "Unable to retrieve current device sample rate. Error = %s", GetError(ret).c_str());
    return 0.0f;
  }
  return sampleRate;
}

bool CCoreAudioDevice::SetNominalSampleRate(Float64 sampleRate)
{
  if (!m_DeviceId || sampleRate == 0.0f)
    return false;

  Float64 currentRate = GetNominalSampleRate();
  if (currentRate == sampleRate)
    return true; //No need to change

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;

  OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, sizeof(Float64), &sampleRate);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetNominalSampleRate: "
      "Unable to set current device sample rate to %0.0f. Error = %s",
      (float)sampleRate, GetError(ret).c_str());
    return false;
  }
  if (m_SampleRateRestore == 0.0f)
    m_SampleRateRestore = currentRate;

  return true;
}

UInt32 CCoreAudioDevice::GetNumLatencyFrames()
{
  UInt32 num_latency_frames = 0;
  if (!m_DeviceId)
    return 0;

  // number of frames of latency in the AudioDevice
  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyLatency;

  UInt32 i_param = 0;
  UInt32 i_param_size = sizeof(uint32_t);
  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &i_param_size, &i_param);
  if (ret == noErr)
    num_latency_frames += i_param;

  // number of frames in the IO buffers
  propertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &i_param_size, &i_param);
  if (ret == noErr)
    num_latency_frames += i_param;

  // number for frames in ahead the current hardware position that is safe to do IO
  propertyAddress.mSelector = kAudioDevicePropertySafetyOffset;
  ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &i_param_size, &i_param);
  if (ret == noErr)
    num_latency_frames += i_param;

  return (num_latency_frames);
}

UInt32 CCoreAudioDevice::GetBufferSize()
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;

  UInt32 size = 0;
  UInt32 propertySize = sizeof(size);
  OSStatus ret = AudioObjectGetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, &propertySize, &size);
  if (ret != noErr)
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetBufferSize: "
      "Unable to retrieve buffer size. Error = %s", GetError(ret).c_str());
  return size;
}

bool CCoreAudioDevice::SetBufferSize(UInt32 size)
{
  if (!m_DeviceId)
    return false;

  AudioObjectPropertyAddress  propertyAddress;
  propertyAddress.mScope    = kAudioDevicePropertyScopeOutput;
  propertyAddress.mElement  = 0;
  propertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;

  UInt32 propertySize = sizeof(size);
  OSStatus ret = AudioObjectSetPropertyData(m_DeviceId, &propertyAddress, 0, NULL, propertySize, &size);
  if (ret != noErr)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetBufferSize: "
      "Unable to set buffer size. Error = %s", GetError(ret).c_str());
  }

  if (GetBufferSize() != size)
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetBufferSize: Buffer size change not applied.");

  return (ret == noErr);
}

XbmcThreads::EndTime CCoreAudioDevice::m_callbackSuppressTimer;
AudioObjectPropertyListenerProc CCoreAudioDevice::m_defaultOutputDeviceChangedCB = NULL;


OSStatus CCoreAudioDevice::defaultOutputDeviceChanged(AudioObjectID                       inObjectID,
                         UInt32                              inNumberAddresses,
                         const AudioObjectPropertyAddress    inAddresses[],
                         void*                               inClientData)
{
  if (m_callbackSuppressTimer.IsTimePast() && m_defaultOutputDeviceChangedCB != NULL)
    return m_defaultOutputDeviceChangedCB(inObjectID, inNumberAddresses, inAddresses, inClientData);
  return 0;
}

void CCoreAudioDevice::RegisterDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback, void *ref)
{
    OSStatus ret = noErr;
    AudioObjectPropertyAddress inAdr =
    {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    if (bRegister)
        ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &inAdr, callback, ref);
    else
        ret = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &inAdr, callback, ref);
    
    if (ret != noErr)
        CLog::Log(LOGERROR, "CCoreAudioAE::Deinitialize - error %s a listener callback for device changes!", bRegister?"attaching":"removing");
}

void CCoreAudioDevice::RegisterDefaultOutputDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback, void *ref)
{
    OSStatus ret = noErr;
    static int registered = -1;
    
    //only allow registration once
    if (bRegister == (registered == 1))
        return;
    
    AudioObjectPropertyAddress inAdr =
    {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    if (bRegister)
    {
        ret = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &inAdr, defaultOutputDeviceChanged, ref);
        m_defaultOutputDeviceChangedCB = callback;
    }
    else
    {
        ret = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &inAdr, defaultOutputDeviceChanged, ref);
        m_defaultOutputDeviceChangedCB = NULL;
    }
    
    if (ret != noErr)
        CLog::Log(LOGERROR, "CCoreAudioAE::Deinitialize - error %s a listener callback for default output device changes!", bRegister?"attaching":"removing");
    else
        registered = bRegister ? 1 : 0;
}

