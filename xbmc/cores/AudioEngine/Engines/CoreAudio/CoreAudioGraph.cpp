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

#include "CoreAudioGraph.h"

#include "CoreAudioAEHAL.h"
#include "CoreAudioMixMap.h"
#include "CoreAudioUnit.h"

#include "utils/log.h"
#include "utils/SystemInfo.h"

CCoreAudioGraph::CCoreAudioGraph() :
  m_audioGraph    (NULL ),
  m_inputUnit     (NULL ),
  m_audioUnit     (NULL ),
  m_mixerUnit     (NULL ),
  m_initialized   (false),
  m_deviceId      (NULL ),
  m_allowMixing   (false),
  m_mixMap        (NULL )
{
  for (int i = 0; i < MAX_CONNECTION_LIMIT; i++)
  {
    m_reservedBusNumber[i] = false;
  }
}

CCoreAudioGraph::~CCoreAudioGraph()
{
  Close();

  delete m_mixMap;
}

bool CCoreAudioGraph::Open(ICoreAudioSource *pSource, AEAudioFormat &format,
  AudioDeviceID deviceId, bool allowMixing, AudioChannelLayoutTag layoutTag, float initVolume)
{
  AudioStreamBasicDescription fmt = {0};
  AudioStreamBasicDescription inputFormat = {0};
  AudioStreamBasicDescription outputFormat = {0};

  m_deviceId = deviceId;
  m_allowMixing = allowMixing;

  OSStatus ret = NewAUGraph(&m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error create audio grpah. Error = %s", GetError(ret).c_str());
    return false;
  }
  ret = AUGraphOpen(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error open audio grpah. Error = %s", GetError(ret).c_str());
    return false;
  }

  // get output unit
  if (m_audioUnit)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error audio unit already open. double call ?");
    return false;
  }

  m_audioUnit = new CAUOutputDevice();
  if (!m_audioUnit->Open(m_audioGraph,
    kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple))
    return false;
  m_audioUnit->SetBus(GetFreeBus());

  m_audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  if (!m_audioUnit->EnableInputOuput())
    return false;

  if (!m_audioUnit->SetCurrentDevice(deviceId))
    return false;

  SetCurrentVolume(initVolume);

  if (allowMixing)
  {
    delete m_mixMap;
    m_mixMap = CCoreAudioMixMap::CreateMixMap(m_audioUnit, format, layoutTag);

    if (m_mixMap || m_mixMap->IsValid())
    {
      // maximum input channel ber input bus
      //fmt.mChannelsPerFrame = MAXIMUM_MIXER_CHANNELS;

      // get output unit
      if (m_inputUnit)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error mixer unit already open. double call ?");
        return false;
      }

      m_inputUnit = new CAUOutputDevice();

      if (!m_inputUnit->Open(m_audioGraph,
        kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple))
        return false;

      if (!m_inputUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
        return false;

      if (!m_inputUnit->SetFormat(&fmt, kAudioUnitScope_Output, kOutputBus))
        return false;

      // get mixer unit
      if (m_mixerUnit)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: Error mixer unit already open. double call ?");
        return false;
      }

      m_mixerUnit = new CAUMatrixMixer();

      if (!m_mixerUnit->Open(m_audioGraph,
        kAudioUnitType_Mixer, kAudioUnitSubType_MatrixMixer, kAudioUnitManufacturer_Apple))
        return false;

      // set number of input buses
      if (!m_mixerUnit->SetInputBusCount(MAX_CONNECTION_LIMIT))
        return false;

      // set number of output buses
      if (!m_mixerUnit->SetOutputBusCount(1))
        return false;

      if (!m_mixerUnit->SetInputBusFormat(MAX_CONNECTION_LIMIT, &fmt))
        return false;

      if (!m_mixerUnit->SetFormat(&fmt, kAudioUnitScope_Output, kOutputBus))
        return false;

      ret =  AUGraphConnectNodeInput(m_audioGraph, m_mixerUnit->GetNode(), 0, m_audioUnit->GetNode(), 0);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
          "Error connecting m_m_mixerNode. Error = %s", GetError(ret).c_str());
        return false;
      }

      m_mixerUnit->SetBus(0);

      // configure output unit
      int busNumber = GetFreeBus();

      ret = AUGraphConnectNodeInput(m_audioGraph, m_inputUnit->GetNode(), 0, m_mixerUnit->GetNode(), busNumber);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
          "Error connecting m_converterNode. Error = %s", GetError(ret).c_str());
        return false;
      }

      m_inputUnit->SetBus(busNumber);

      ret = AUGraphUpdate(m_audioGraph, NULL);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
          "Error update graph. Error = %s", GetError(ret).c_str());
        return false;
      }
      ret = AUGraphInitialize(m_audioGraph);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
          "Error initialize graph. Error = %s", GetError(ret).c_str());
        return false;
      }

      // Update format structure to reflect the desired format from the mixer
      // The output format of the mixer is identical to the input format, except for the channel count
      fmt.mChannelsPerFrame = m_mixMap->GetOutputChannels();

      UInt32 inputNumber = m_inputUnit->GetBus();
      int channelOffset = GetMixerChannelOffset(inputNumber);
      if (!CCoreAudioMixMap::SetMixingMatrix(m_mixerUnit, m_mixMap, &inputFormat, &fmt, channelOffset))
        return false;

      // Regenerate audio format and copy format for the Output AU
      outputFormat = fmt;
    }
    else
    {
      outputFormat = inputFormat;
    }

  }
  else
  {
    outputFormat = inputFormat;
  }

  if (!m_audioUnit->SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error setting input format on audio device. Channel count %d, set it to %d",
      (int)outputFormat.mChannelsPerFrame, format.m_channelLayout.Count());
    outputFormat.mChannelsPerFrame = format.m_channelLayout.Count();
    if (!m_audioUnit->SetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
      return false;
  }

  std::string formatString;
  // asume we are in dd-wave mode
  if (!m_inputUnit)
  {
    if (!m_audioUnit->SetFormat(&inputFormat, kAudioUnitScope_Output, kInputBus))
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
        "Error setting Device Output Stream Format %s",
        StreamDescriptionToString(inputFormat, formatString));
    }
  }

  ret = AUGraphUpdate(m_audioGraph, NULL);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error update graph. Error = %s", GetError(ret).c_str());
    return false;
  }

  AudioStreamBasicDescription inputDesc_end, outputDesc_end;
  m_audioUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
  m_audioUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kInputBus);
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: audioUnit, Input Stream Format  %s",
    StreamDescriptionToString(inputDesc_end, formatString));
  CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: audioUnit, Output Stream Format %s",
    StreamDescriptionToString(outputDesc_end, formatString));

  if (m_mixerUnit)
  {
    m_mixerUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
    m_mixerUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kOutputBus);
    CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: mixerUnit, Input Stream Format  %s",
      StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: mixerUnit, Output Stream Format %s",
      StreamDescriptionToString(outputDesc_end, formatString));
  }

  if (m_inputUnit)
  {
    m_inputUnit->GetFormat(&inputDesc_end, kAudioUnitScope_Input, kOutputBus);
    m_inputUnit->GetFormat(&outputDesc_end, kAudioUnitScope_Output, kOutputBus);
    CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: inputUnit, Input Stream Format  %s",
      StreamDescriptionToString(inputDesc_end, formatString));
    CLog::Log(LOGDEBUG, "CCoreAudioGraph::Open: inputUnit, Output Stream Format %s",
      StreamDescriptionToString(outputDesc_end, formatString));
  }

  ret = AUGraphInitialize(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Open: "
      "Error initialize graph. Error = %s", GetError(ret).c_str());
    return false;
  }

  UInt32 bufferFrames = m_audioUnit->GetBufferFrameSize();
  if (!m_audioUnit->SetMaxFramesPerSlice(bufferFrames))
    return false;

  SetInputSource(pSource);

  return Start();
}

bool CCoreAudioGraph::Close()
{
  if (!m_audioGraph)
    return false;

  Stop();

  SetInputSource(NULL);

  while (!m_auUnitList.empty())
  {
    CAUOutputDevice *d = m_auUnitList.front();
    m_auUnitList.pop_front();
    ReleaseBus(d->GetBus());
    d->SetInputSource(NULL);
    d->Close();
    delete d;
  }

  if (m_inputUnit)
  {
    ReleaseBus(m_inputUnit->GetBus());
    m_inputUnit->Close();
    delete m_inputUnit;
    m_inputUnit = NULL;
  }

  if (m_mixerUnit)
  {
    m_mixerUnit->Close();
    delete m_mixerUnit;
    m_mixerUnit = NULL;
  }

  if (m_audioUnit)
  {
    m_audioUnit->Close();
    delete m_audioUnit;
    m_audioUnit = NULL;
  }

  OSStatus ret = AUGraphUninitialize(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: "
      "Error unitialize. Error = %s", GetError(ret).c_str());
  }

  ret = AUGraphClose(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: "
      "Error close. Error = %s", GetError(ret).c_str());
  }

  ret = DisposeAUGraph(m_audioGraph);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Close: "
      "Error dispose. Error = %s", GetError(ret).c_str());
  }

  return true;
}

bool CCoreAudioGraph::Start()
{
  if (!m_audioGraph)
    return false;

  Boolean isRunning = false;
  OSStatus ret = AUGraphIsRunning(m_audioGraph, &isRunning);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::Start: "
      "Audio graph not running. Error = %s", GetError(ret).c_str());
    return false;
  }
  if (!isRunning)
  {
    if (m_audioUnit)
      m_audioUnit->Start();
    if (m_mixerUnit)
      m_mixerUnit->Start();
    if (m_inputUnit)
      m_inputUnit->Start();

    ret = AUGraphStart(m_audioGraph);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Start: "
        "Error starting audio graph. Error = %s", GetError(ret).c_str());
    }
  }

  return true;
}

bool CCoreAudioGraph::Stop()
{
  if (!m_audioGraph)
    return false;

  Boolean isRunning = false;
  OSStatus ret = AUGraphIsRunning(m_audioGraph, &isRunning);
  if (ret)
  {
    if (m_inputUnit)
      m_inputUnit->Stop();
    if (m_mixerUnit)
      m_mixerUnit->Stop();
    if (m_audioUnit)
      m_audioUnit->Stop();

    CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: "
      "Audio graph not running. Error = %s", GetError(ret).c_str());
    return false;
  }
  if (isRunning)
  {
    ret = AUGraphStop(m_audioGraph);
    if (ret)
    {
      CLog::Log(LOGERROR, "CCoreAudioGraph::Stop: "
        "Error stopping audio graph. Error = %s", GetError(ret).c_str());
    }
  }

  return true;
}

AudioChannelLayoutTag CCoreAudioGraph::GetChannelLayoutTag(int layout)
{
  return g_LayoutMap[layout];
}

bool CCoreAudioGraph::SetInputSource(ICoreAudioSource* pSource)
{
  if (m_inputUnit)
    return m_inputUnit->SetInputSource(pSource);
  else if (m_audioUnit)
    return m_audioUnit->SetInputSource(pSource);

  return false;
}

bool CCoreAudioGraph::SetCurrentVolume(Float32 vol)
{
  if (!m_audioUnit)
    return false;

  return m_audioUnit->SetCurrentVolume(vol);
}

CAUOutputDevice *CCoreAudioGraph::DestroyUnit(CAUOutputDevice *outputUnit)
{
  if (!outputUnit)
    return NULL;

  Stop();

  for (AUUnitList::iterator itt = m_auUnitList.begin(); itt != m_auUnitList.end(); ++itt)
  {
    if (*itt == outputUnit)
    {
      m_auUnitList.erase(itt);
      break;
    }
  }

  ReleaseBus(outputUnit->GetBus());
  outputUnit->SetInputSource(NULL);
  outputUnit->Close();
  delete outputUnit;
  outputUnit = NULL;

  AUGraphUpdate(m_audioGraph, NULL);

  Start();

  return NULL;
}

CAUOutputDevice *CCoreAudioGraph::CreateUnit(AEAudioFormat &format)
{
  if (!m_audioUnit || !m_mixerUnit)
    return NULL;

  AudioStreamBasicDescription fmt = {0};
  AudioStreamBasicDescription inputFormat = {0};
  AudioStreamBasicDescription outputFormat = {0};

  int busNumber = GetFreeBus();
  if (busNumber == INVALID_BUS)
    return  NULL;

  OSStatus ret;
  // create output unit
  CAUOutputDevice *outputUnit = new CAUOutputDevice();
  if (!outputUnit->Open(m_audioGraph,
    kAudioUnitType_FormatConverter, kAudioUnitSubType_AUConverter, kAudioUnitManufacturer_Apple))
    goto error;

  m_audioUnit->GetFormatDesc(format, &inputFormat, &fmt);

  // get the format frm the mixer
  if (!m_mixerUnit->GetFormat(&outputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&inputFormat, kAudioUnitScope_Input, kOutputBus))
    goto error;

  if (!outputUnit->SetFormat(&outputFormat, kAudioUnitScope_Output, kOutputBus))
    goto error;

  ret = AUGraphConnectNodeInput(m_audioGraph, outputUnit->GetNode(), 0, m_mixerUnit->GetNode(), busNumber);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioGraph::CreateUnit: "
      "Error connecting outputUnit. Error = %s", GetError(ret).c_str());
    goto error;
  }

  // TODO: setup mixmap, get free bus number for connection

  outputUnit->SetBus(busNumber);

  if (m_mixMap || m_mixMap->IsValid())
  {
    UInt32 inputNumber = outputUnit->GetBus();
    int channelOffset = GetMixerChannelOffset(inputNumber);
    CCoreAudioMixMap::SetMixingMatrix(m_mixerUnit, m_mixMap, &inputFormat, &fmt, channelOffset);
  }

  AUGraphUpdate(m_audioGraph, NULL);
  m_auUnitList.push_back(outputUnit);

  return outputUnit;

error:
  delete outputUnit;
  return NULL;
}

int CCoreAudioGraph::GetFreeBus()
{
  for (int i = 0; i < MAX_CONNECTION_LIMIT; i++)
  {
    if (!m_reservedBusNumber[i])
    {
      m_reservedBusNumber[i] = true;
      return i;
    }
  }
  return INVALID_BUS;
}

void CCoreAudioGraph::ReleaseBus(int busNumber)
{
  if (busNumber > MAX_CONNECTION_LIMIT || busNumber < 0)
    return;

  m_reservedBusNumber[busNumber] = false;
}

bool CCoreAudioGraph::IsBusFree(int busNumber)
{
  if (busNumber > MAX_CONNECTION_LIMIT || busNumber < 0)
    return false;
  return m_reservedBusNumber[busNumber];
}

int CCoreAudioGraph::GetMixerChannelOffset(int busNumber)
{
  if (!m_mixerUnit)
    return 0;

  int offset = 0;
  AudioStreamBasicDescription fmt = {0};

  for (int i = 0; i < busNumber; i++)
  {
    memset(&fmt, 0x0, sizeof(fmt));
    m_mixerUnit->GetFormat(&fmt, kAudioUnitScope_Input, busNumber);
    offset += fmt.mChannelsPerFrame;
  }
  return offset;
}
