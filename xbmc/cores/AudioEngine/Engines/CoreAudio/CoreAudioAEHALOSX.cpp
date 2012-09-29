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

#ifdef TARGET_DARWIN_OSX

#include "system.h"

#include "CoreAudioAEHALOSX.h"

#include "CoreAudioAE.h"
#include "CoreAudioAEHAL.h"
#include "CoreAudioUnit.h"
#include "CoreAudioDevice.h"
#include "CoreAudioGraph.h"
#include "CoreAudioMixMap.h"
#include "CoreAudioHardware.h"
#include "CoreAudioChannelLayout.h"

#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/GUISettings.h"
#include "utils/log.h"

CCoreAudioAEHALOSX::CCoreAudioAEHALOSX() :
  m_audioGraph        (NULL   ),
  m_Initialized       (false  ),
  m_Passthrough       (false  ),
  m_allowMixing       (false  ),
  m_encoded           (false  ),
  m_initVolume        (1.0f   ),
  m_NumLatencyFrames  (0      ),
  m_OutputBufferIndex (0      )
{
  m_AudioDevice   = new CCoreAudioDevice();
  m_OutputStream  = new CCoreAudioStream();

  SInt32 major, minor;
  Gestalt(gestaltSystemVersionMajor, &major);
  Gestalt(gestaltSystemVersionMinor, &minor);

  // By default, kAudioHardwarePropertyRunLoop points at the process's main thread on SnowLeopard,
  // If your process lacks such a run loop, you can set kAudioHardwarePropertyRunLoop to NULL which
  // tells the HAL to run it's own thread for notifications (which was the default prior to SnowLeopard).
  // So tell the HAL to use its own thread for similar behavior under all supported versions of OSX.
  if (major == 10 && minor >= 6)
  {
    CFRunLoopRef theRunLoop = NULL;
    AudioObjectPropertyAddress theAddress = {
      kAudioHardwarePropertyRunLoop,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
    };
    OSStatus theError = AudioObjectSetPropertyData(kAudioObjectSystemObject,
      &theAddress, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
    if (theError != noErr)
    {
      CLog::Log(LOGERROR, "CCoreAudioAE::constructor: kAudioHardwarePropertyRunLoop error.");
    }
  }
}

CCoreAudioAEHALOSX::~CCoreAudioAEHALOSX()
{
  Deinitialize();

  delete m_audioGraph;
  delete m_AudioDevice;
  delete m_OutputStream;
}

bool CCoreAudioAEHALOSX::InitializePCM(ICoreAudioSource *pSource, AEAudioFormat &format, bool allowMixing, AudioDeviceID outputDevice)
{
  if (m_audioGraph)
    m_audioGraph->Close(), delete m_audioGraph;
  m_audioGraph = new CCoreAudioGraph();
  if (!m_audioGraph)
    return false;

  AudioChannelLayoutTag layout = g_LayoutMap[ g_guiSettings.GetInt("audiooutput.channellayout") ];
  // force optical/coax to 2.0 output channels
  if (!m_Passthrough && g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958)
    layout = g_LayoutMap[1];

  if (!m_audioGraph->Open(pSource, format, outputDevice, allowMixing, layout, m_initVolume ))
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: "
      "Unable to initialize audio due a missconfiguration. Try 2.0 speaker configuration.");
    return false;
  }

  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();

  m_allowMixing = allowMixing;

  return true;
}

bool CCoreAudioAEHALOSX::InitializePCMEncoded(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID outputDevice)
{
  // Prevent any other application from using this device.
  m_AudioDevice->SetHogStatus(true);
  // Try to disable mixing support. Effectiveness depends on the device.
  m_AudioDevice->SetMixingSupport(false);
  // Set the Sample Rate as defined by the spec.
  m_AudioDevice->SetNominalSampleRate((float)format.m_sampleRate);

  if (!InitializePCM(pSource, format, false, outputDevice))
    return false;

  return true;
}

bool CCoreAudioAEHALOSX::InitializeEncoded(AudioDeviceID outputDevice, AEAudioFormat &format)
{
  std::string formatString;
  AudioStreamID outputStream = 0;
  AudioStreamBasicDescription outputFormat = {0};

  // Fetch a list of the streams defined by the output device
  UInt32 streamIndex = 0;
  AudioStreamIdList streams;
  m_AudioDevice->GetStreams(&streams);

  m_OutputBufferIndex = 0;

  while (!streams.empty())
  {
    // Get the next stream
    CCoreAudioStream stream;
    stream.Open(streams.front());
    streams.pop_front(); // We copied it, now we are done with it

    // Probe physical formats
    StreamFormatList physicalFormats;
    stream.GetAvailablePhysicalFormats(&physicalFormats);
    while (!physicalFormats.empty())
    {
      AudioStreamRangedDescription& desc = physicalFormats.front();
      CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded:    "
        "Considering Physical Format: %s", StreamDescriptionToString(desc.mFormat, formatString));

      if (m_rawDataFormat == AE_FMT_LPCM   || m_rawDataFormat == AE_FMT_DTSHD ||
          m_rawDataFormat == AE_FMT_TRUEHD || m_rawDataFormat == AE_FMT_EAC3)
      {
        // check pcm output formats
        unsigned int bps = CAEUtil::DataFormatToBits(AE_FMT_S16NE);
        if (desc.mFormat.mChannelsPerFrame == m_initformat.m_channelLayout.Count() &&
            desc.mFormat.mBitsPerChannel == bps &&
            desc.mFormat.mSampleRate == m_initformat.m_sampleRate )
        {
          outputFormat = desc.mFormat; // Select this format
          m_OutputBufferIndex = streamIndex;
          outputStream = stream.GetId();
          break;
        }
      }
      else
      {
        // check encoded formats
        if (desc.mFormat.mFormatID == kAudioFormat60958AC3 || desc.mFormat.mFormatID == 'IAC3')
        {
          if (desc.mFormat.mChannelsPerFrame == m_initformat.m_channelLayout.Count() &&
              desc.mFormat.mSampleRate == m_initformat.m_sampleRate )
          {
            outputFormat = desc.mFormat; // Select this format
            m_OutputBufferIndex = streamIndex;
            outputStream = stream.GetId();
            break;
          }
        }
      }
      physicalFormats.pop_front();
    }

    // TODO: How do we determine if this is the right stream (not just the right format) to use?
    if (outputFormat.mFormatID)
      break; // We found a suitable format. No need to continue.
    streamIndex++;
  }

  if (!outputFormat.mFormatID) // No match found
  {
    CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
      "Unable to identify suitable output format.");
    return false;
  }

  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
    "Selected stream[%u] - id: 0x%04X, Physical Format: %s",
    m_OutputBufferIndex, (uint)outputStream, StreamDescriptionToString(outputFormat, formatString));

  // TODO: Auto hogging sets this for us. Figure out how/when to turn it off or use it
  // It appears that leaving this set will aslo restore the previous stream format when the
  // Application exits. If auto hogging is set and we try to set hog mode, we will deadlock
  // From the SDK docs: "If the AudioDevice is in a non-mixable mode, the HAL will automatically take hog mode on behalf of the first process to start an IOProc."

  // Lock down the device.  This MUST be done PRIOR to switching to a non-mixable format, if it is done at all
  // If it is attempted after the format change, there is a high likelihood of a deadlock
  // We may need to do this sooner to enable mix-disable (i.e. before setting the stream format)

  // Auto-Hog does not always un-hog the device when changing back to a mixable mode.
  // Handle this on our own until it is fixed.
  CCoreAudioHardware::SetAutoHogMode(false);
  bool autoHog = CCoreAudioHardware::GetAutoHogMode();
  CLog::Log(LOGDEBUG, " CoreAudioRenderer::InitializeEncoded: "
    "Auto 'hog' mode is set to '%s'.", autoHog ? "On" : "Off");
  if (!autoHog) // Try to handle this ourselves
  {
    // Hog the device if it is not set to be done automatically
    m_AudioDevice->SetHogStatus(true);
    // Try to disable mixing. If we cannot, it may not be a problem
    m_AudioDevice->SetMixingSupport(false);
  }

  m_NumLatencyFrames = m_AudioDevice->GetNumLatencyFrames();

  // Configure the output stream object, this is the one we will keep
  m_OutputStream->Open(outputStream);

  AudioStreamBasicDescription virtualFormat;
  m_OutputStream->GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
    "Previous Virtual Format: %s", StreamDescriptionToString(virtualFormat, formatString));

  AudioStreamBasicDescription previousPhysicalFormat;
  m_OutputStream->GetPhysicalFormat(&previousPhysicalFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
    "Previous Physical Format: %s", StreamDescriptionToString(previousPhysicalFormat, formatString));

  // Set the active format (the old one will be reverted when we close)
  m_OutputStream->SetPhysicalFormat(&outputFormat);
  m_NumLatencyFrames += m_OutputStream->GetNumLatencyFrames();

  m_OutputStream->GetVirtualFormat(&virtualFormat);
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
    "New Virtual Format: %s", StreamDescriptionToString(virtualFormat, formatString));
  CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::InitializeEncoded: "
    "New Physical Format: %s", StreamDescriptionToString(outputFormat, formatString));

  m_allowMixing = false;

  return true;
}

bool CCoreAudioAEHALOSX::Initialize(ICoreAudioSource *ae, bool passThrough, AEAudioFormat &format, AEDataFormat rawDataFormat, std::string &device, float initVolume)
{
  // Reset all the devices to a default 'non-hog' and mixable format.
  // If we don't do this we may be unable to find the Default Output device.
  // (e.g. if we crashed last time leaving it stuck in AC-3 mode)

  CCoreAudioHardware::ResetAudioDevices();

  m_ae = (CCoreAudioAE*)ae;
  if (!m_ae)
    return false;

  m_initformat          = format;
  m_rawDataFormat       = rawDataFormat;
  m_Passthrough         = passThrough;
  m_encoded             = false;
  m_OutputBufferIndex   = 0;
  m_initVolume          = initVolume;

  if (format.m_channelLayout.Count() == 0)
  {
    CLog::Log(LOGERROR, "CCoreAudioAEHALOSX::Initialize - "
      "Unable to open the requested channel layout");
    return false;
  }

  if (device.find("CoreAudio:") != std::string::npos)
    device.erase(0, strlen("CoreAudio:"));

  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(device);
  if (!outputDevice)
  {
    // Fall back to the default device if no match is found
    CLog::Log(LOGWARNING, "CCoreAudioAEHALOSX::Initialize: "
      "Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device. TODO: Should we just grab the first existing device?
      return false;
  }

  // Attach our output object to the device
  m_AudioDevice->Open(outputDevice);
  m_AudioDevice->SetHogStatus(false);
  m_AudioDevice->SetMixingSupport(true);

  // If this is a passthrough (AC3/DTS) stream, attempt to handle it natively
  if (m_Passthrough)
    m_encoded = InitializeEncoded(outputDevice, format);

  // If this is a PCM stream, or we failed to handle a passthrough stream natively,
  // prepare the standard interleaved PCM interface
  if (!m_encoded)
  {
    // If we are here and this is a passthrough stream, native handling failed.
    // Try to handle it as IEC61937 data over straight PCM (DD-Wav)
    bool configured = false;
    if (m_Passthrough)
    {
      CLog::Log(LOGDEBUG, "CCoreAudioAEHALOSX::Initialize: "
        "No suitable AC3 output format found. Attempting DD-Wav.");
      configured = InitializePCMEncoded(ae, format, outputDevice);
    }
    else
    {
      // Standard PCM data
      configured = InitializePCM(ae, format, true, outputDevice);
    }

    // No suitable output format was able to be configured
    if (!configured)
      return false;
  }

  m_Initialized = true;

  return true;
}

CAUOutputDevice *CCoreAudioAEHALOSX::DestroyUnit(CAUOutputDevice *outputUnit)
{
  if (m_audioGraph && outputUnit)
    return m_audioGraph->DestroyUnit(outputUnit);

  return NULL;
}

CAUOutputDevice *CCoreAudioAEHALOSX::CreateUnit(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  CAUOutputDevice *outputUnit = NULL;

  // when HAL is using a mixer, the input is routed through converter units.
  // therefore we create a converter unit attach the source and give it back.
  if (m_allowMixing && m_audioGraph)
  {
    outputUnit = m_audioGraph->CreateUnit(format);

    if (pSource && outputUnit)
      outputUnit->SetInputSource(pSource);
  }

  return outputUnit;
}

void CCoreAudioAEHALOSX::Deinitialize()
{
  if (!m_Initialized)
    return;

  Stop();

  if (m_encoded)
    m_AudioDevice->SetInputSource(NULL, 0, 0);

  if (m_audioGraph)
    m_audioGraph->SetInputSource(NULL);

  m_OutputStream->Close();
  m_AudioDevice->Close();

  if (m_audioGraph)
  {
    //m_audioGraph->Close();
    delete m_audioGraph;
  }
  m_audioGraph = NULL;

  m_NumLatencyFrames = 0;
  m_OutputBufferIndex = 0;

  m_Initialized = false;
  m_Passthrough = false;
}

void CCoreAudioAEHALOSX::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  CoreAudioDeviceList deviceList;
  CCoreAudioHardware::GetOutputDevices(&deviceList);

  std::string defaultDeviceName;
  CCoreAudioHardware::GetOutputDeviceName(defaultDeviceName);

  std::string deviceName;
  for (int i = 0; !deviceList.empty(); i++)
  {
    CCoreAudioDevice device(deviceList.front());
    deviceName = device.GetName();

    std::string deviceName_Internal = std::string("CoreAudio:");
    deviceName_Internal.append(deviceName);
    devices.push_back(AEDevice(deviceName, deviceName_Internal));

    deviceList.pop_front();
  }
}

void CCoreAudioAEHALOSX::Stop()
{
  if (!m_Initialized)
    return;

  if (m_encoded)
    m_AudioDevice->Stop();
  else
    m_audioGraph->Stop();
}

bool CCoreAudioAEHALOSX::Start()
{
  if (!m_Initialized)
    return false;

  if (m_encoded)
    m_AudioDevice->Start();
  else
    m_audioGraph->Start();

  return true;
}

void CCoreAudioAEHALOSX::SetDirectInput(ICoreAudioSource *pSource, AEAudioFormat &format)
{
  if (!m_Initialized)
    return;

  // when HAL is initialized encoded we use directIO
  // when HAL is not in encoded mode and there is no mixer attach source the audio unit
  // when mixing is allowed in HAL, HAL is working with converter units where we attach the source.

  if (m_encoded)
  {
    // register directcallback for the audio HAL
    // direct render callback need to know the framesize and buffer index
    if (pSource)
      m_AudioDevice->SetInputSource(pSource, format.m_frameSize, m_OutputBufferIndex);
    else
      m_AudioDevice->SetInputSource(pSource, 0, 0);
  }
  else if (!m_encoded && !m_allowMixing)
  {
    // register render callback for the audio unit
    m_audioGraph->SetInputSource(pSource);
  }
}

double CCoreAudioAEHALOSX::GetDelay()
{
  return (double)(m_NumLatencyFrames) / (m_initformat.m_sampleRate);
}

void CCoreAudioAEHALOSX::SetVolume(float volume)
{
  if (m_encoded || m_Passthrough)
    return;

  m_audioGraph->SetCurrentVolume(volume);
}

unsigned int CCoreAudioAEHALOSX::GetBufferIndex()
{
  return m_OutputBufferIndex;
}

#endif
