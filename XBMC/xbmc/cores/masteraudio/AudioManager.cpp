/*
 *      Copyright (C) 2009 Team XBMC
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

#include "stdafx.h"
#include "AudioManager.h"

// Global (ick) Instance
CAudioManager g_AudioLibManager;

CAudioStream::CAudioStream() :
  m_pInput(NULL),
  m_pDSPChain(NULL),
  m_MixerChannel(0),
  m_pMixerSink(NULL), // DO NOT 'delete' the sink!
  m_Open(false)
{
  m_ProcessTimer.reset();
  m_IntervalTimer.reset();
}

CAudioStream::~CAudioStream()
{
    Close();
}

bool CAudioStream::Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, int mixerChannel, IAudioSink* pMixerSink)
{
  if (m_Open || !pInput || !pDSPChain || !mixerChannel || !pMixerSink)
    return false;

  m_pInput = pInput;
  m_pDSPChain = pDSPChain;
  m_MixerChannel = mixerChannel;
  m_pMixerSink = pMixerSink;

  // Hook-up interconnections: Input -> DSPChain, DSPChain -> Mixer
  // It is assumed at this point that the input/output stream formats are compatible
  if ((MA_SUCCESS != m_InputConnection.Link(m_pInput->GetSource(),m_pDSPChain->GetSink()) ||
   (MA_SUCCESS != m_OutputConnection.Link(m_pDSPChain->GetSource(),m_pMixerSink))))
    return false; // There was a problem linking the sink/source

  m_Open = true;
  return m_Open;
}

void CAudioStream::Close()
{
  if (!m_Open)
    return;

  // Break down any data connections
  m_Open = false;
  m_InputConnection.Unlink();
  m_OutputConnection.Unlink();

  CLog::Log(LOGINFO,"MasterAudio:AudioStream: Closing. Average time to process = %0.2fms / %0.2fms (%0.2f%% duty cycle)", m_ProcessTimer.average_time/1000.0f, m_IntervalTimer.average_time/1000.0f, (m_ProcessTimer.average_time / m_IntervalTimer.average_time) * 100);
}

CStreamInput* CAudioStream::GetInput()
{
  return m_pInput;
}

CDSPChain* CAudioStream::GetDSPChain()
{
  return m_pDSPChain;
}

int CAudioStream::GetMixerChannel()
{
  return m_MixerChannel;
}

bool CAudioStream::ProcessStream()
{
  m_IntervalTimer.lap_end();
  // TODO: Consider multiple calls to fill downstream buffers
  bool ret = true;
  MA_RESULT res = MA_SUCCESS;
  
  m_ProcessTimer.lap_start();

  m_InputConnection.Process();
  m_OutputConnection.Process();

  m_ProcessTimer.lap_end();

  m_IntervalTimer.lap_start(); // Time between calls
  return ret;
}

float CAudioStream::GetMaxLatency()
{
  // Latency has 4 parts
  //  1. Input/Output buffer (TODO: no interface)
  //  2. DSPChain
  //  3. Mixer Channel(renderer)
  //  4. Interconnect buffers (TODO: currently cannot reliably convert bytes to time)

  // TODO: Periodically cache this value and provide average as opposed to re-calculating each time. It's only
  //     so accurate anyway, since the latency now may not equal the latency experienced by any given byte
  return m_pDSPChain->GetMaxLatency() + m_pMixerSink->GetMaxLatency();
}

void CAudioStream::Flush()
{
  // Empty the input buffer
  if (m_pInput)
    m_pInput->Reset();

  // Flush connections
  m_InputConnection.Flush();
  m_OutputConnection.Flush();

  CLog::Log(LOGINFO,"MasterAudio:AudioStream: Flushed stream.");
}

bool CAudioStream::Drain(unsigned int timeout)
{
  // TODO: Make sure this doesn't leave any data behind
  lap_timer timer;
  timer.lap_start();
  while (timer.elapsed_time()/1000 < timeout)
  {
    // Move any remaining data along
    m_InputConnection.Process();
    m_OutputConnection.Process();
  
    if (!m_InputConnection.GetCacheSize() && !m_OutputConnection.GetCacheSize()) // No input left here. Try to empy out the mixer channel
    {
      // TODO: Drain mixer channel
      m_pInput->Reset();  // Just in case there is a partial slice left
      return true;
    }
  }
  
  Flush();  // Abandon any remaining data
  return false; // We ran out of time
}

///////////////////////////////////////////////////////////////////////////////
// CAudioManager
///////////////////////////////////////////////////////////////////////////////
CAudioManager::CAudioManager() :
  m_pMixer(NULL),
  m_MaxStreams(MA_MAX_INPUT_STREAMS)
{

}

CAudioManager::~CAudioManager()
{
  // Clean up any streams that were not closed by a client
  if (m_StreamList.size())
  {
    for (StreamIterator iter = m_StreamList.begin();iter != m_StreamList.end();iter++)
    {
      CAudioStream* pStream = (CAudioStream*)iter->second;
      m_StreamList.erase(iter);
      CleanupStreamResources(pStream);
      delete pStream;
    }
  }
  delete m_pMixer;
}

MA_STREAM_ID CAudioManager::OpenStream(CStreamDescriptor* pDesc)
{
  // Find out if we can handle an additional input stream, if not, give up
  if (!pDesc || m_StreamList.size() >= m_MaxStreams)
  {
    CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream. Too many active streams. (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
    return MA_STREAM_NONE;
  }

  // Create components for a new AudioStream
  CAudioStream* pStream = new CAudioStream();
  CStreamInput* pInput = new CStreamInput();        // Input Handler
  CDSPChain* pChain = new CDSPChain();              // DSP Handling Chain
  if (!m_pMixer)
    SetMixerType(MA_MIXER_HARDWARE);
  int mixerChannel = 0;
  while (m_pMixer)
  {
    // No need to test the StreamInput output format, as it treats all data the same. Provide it with the descriptor anyway.
    if (MA_SUCCESS != pInput->SetOutputFormat(pDesc))
      break;

    // Fetch the appropriate output profile
    // TODO: Determine which output format to used based on profile
    audio_profile* pProfile = GetProfile(pDesc);
    if (!pProfile)
      break; // We don't know what to do with this stream (there should always be a default, though)

    // TODO: Fetch profile
    //CStreamDescriptor* pOutputDesc = &pProfile->output_descriptor;
    CStreamDescriptor* pOutputDesc = pDesc;

    // Open a mixer channel using the preferred format
    mixerChannel = m_pMixer->OpenChannel(pOutputDesc);  // Virtual Mixer Channel
    if (!mixerChannel)
      break; // TODO: Keep trying until a working format is found or we run out of options

    // Attempt to create the DSP Filter Graph using the identified formats (TODO: Can this be updated on the fly?)
    if (MA_SUCCESS != pChain->CreateFilterGraph(pDesc, pOutputDesc))
      break; // We cannot convert from the input format to the output format

    // We should be good to go. Wrap everything up in a new Stream object
    if (MA_SUCCESS != pStream->Initialize(pInput, pChain, mixerChannel, m_pMixer->GetChannelSink(mixerChannel)))
      break;
    
    // Add the new stream to our collection
    MA_STREAM_ID streamId = GetStreamId(pStream);
    m_StreamList[streamId] = pStream;

    CLog::Log(LOGINFO,"MasterAudio:AudioManager: Opened stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), m_MaxStreams);
    return streamId;
  }

  // Something went wrong.  Clean up and return.
  pStream->Close();
  if (m_pMixer)
    m_pMixer->CloseChannel(mixerChannel);
  delete pStream;
  delete pChain;
  delete pInput;
  
  CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
  return MA_STREAM_NONE;
}

size_t CAudioManager::AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len)
{
  size_t bytesAdded = 0;

  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return 0;

  CStreamInput* pInput = pStream->GetInput();
  if (!pInput)
    return 0;

  // Add the data to the stream
  if(MA_SUCCESS == pInput->AddData(pData,len))
    bytesAdded = len;
  else
    bytesAdded = 0;

  // 'Push' data through the stream
  pStream->ProcessStream();

  return bytesAdded;
}

bool CAudioManager::ControlStream(MA_STREAM_ID streamId, int controlCode)
{
  CAudioStream* pStream = GetInputStream(streamId);
  if(!pStream)
    return false;

  int mixerChannel = pStream->GetMixerChannel();
  if(!mixerChannel)
    return false;

  return (MA_SUCCESS == m_pMixer->ControlChannel(mixerChannel, controlCode));
}

bool CAudioManager::SetStreamVolume(MA_STREAM_ID streamId, long vol)
{
  CAudioStream* pStream = GetInputStream(streamId);
  if(!pStream)
    return false;

  int mixerChannel = pStream->GetMixerChannel();
  if(!mixerChannel)
    return false;

  return (MA_SUCCESS == m_pMixer->SetChannelVolume(mixerChannel, vol));  
}

bool CAudioManager::SetStreamProp(MA_STREAM_ID streamId, int propId, const void* pVal)
{
  // TODO: Implement
  CAudioStream* pStream = GetInputStream(streamId);
  if(!pStream)
    return false;

  return false;
}

bool CAudioManager::GetStreamProp(MA_STREAM_ID streamId, int propId, void* pVal)
{
  // TODO: Implement
  CAudioStream* pStream = GetInputStream(streamId);
  if(!pStream)
    return false;

  return false;
}

float CAudioManager::GetMaxStreamLatency(MA_STREAM_ID streamId)
{
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return 0.0f;  // No delay from here to nowhere...

  return pStream->GetMaxLatency();
}

bool CAudioManager::DrainStream(MA_STREAM_ID streamId, unsigned int timeout)
{
  // TODO: There must be a cleaner way to do this (why can't the AudioStream drain the mixer channel?)
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return true; // Nothing to drain

  lap_timer timer;
  timer.lap_start();
  if (pStream->Drain(timeout))
  {
    unsigned __int64 elapsed = timer.elapsed_time()/1000;
    if (elapsed < timeout)
      return m_pMixer->DrainChannel(pStream->GetMixerChannel(), timeout - (unsigned int)elapsed);
    else
      m_pMixer->FlushChannel(pStream->GetMixerChannel());
  }
  return false; // Out of time
}

void CAudioManager::FlushStream(MA_STREAM_ID streamId)
{
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return;

  pStream->Flush();
}

void CAudioManager::CloseStream(MA_STREAM_ID streamId)
{
  StreamIterator iter = m_StreamList.find(streamId);
  if (iter == m_StreamList.end())
    return;
  CAudioStream* pStream = iter->second;
  m_StreamList.erase(iter);
  CleanupStreamResources(pStream);
  delete pStream;

  CLog::Log(LOGINFO,"MasterAudio:AudioManager: Closed stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), m_MaxStreams);
}

bool CAudioManager::SetMixerType(int mixerType)
{
  if (m_pMixer)
    delete m_pMixer;

  m_pMixer = NULL;

  switch(mixerType)
  {
  case MA_MIXER_HARDWARE:
    m_pMixer = new CHardwareMixer(m_MaxStreams);
    return true;
  case MA_MIXER_SOFTWARE:
  default:
    return false;
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Private Methods
/////////////////////////////////////////////////////////////////////////////////////
CAudioStream* CAudioManager::GetInputStream(MA_STREAM_ID streamId)
{
  return (CAudioStream*)streamId;
}

MA_STREAM_ID CAudioManager::GetStreamId(CAudioStream* pStream)
{
  return (MA_STREAM_ID)pStream;
}

int CAudioManager::GetOpenStreamCount()
{
  return m_StreamList.size();
}

void CAudioManager::CleanupStreamResources(CAudioStream* pStream)
{
  // Clean up any resources created for the stream
  delete pStream->GetInput();
  delete pStream->GetDSPChain();
  m_pMixer->CloseChannel(pStream->GetMixerChannel());
  pStream->Close();
}

audio_profile g_AudioProfileAC3; // TODO: DeleteMe
audio_profile g_AudioProfileStereo; // TODO: DeleteMe
bool g_AudioProfileInit = false; // TODO: DeleteMe

// Audio Profiles are used to define specific audio scenarios based on user configuration, available hardware, and source stream characteristics
audio_profile* CAudioManager::GetProfile(CStreamDescriptor* pInputDesc)
{
 // TODO: Probe input descriptor and select appropriate pre-configured profile

 if (!g_AudioProfileInit)
 {
    // Global AC3 Output profile
    CStreamAttributeCollection* pAtts = g_AudioProfileAC3.output_descriptor.GetAttributes();
    pAtts->SetInt(MA_ATT_TYPE_STREAM_FORMAT,MA_STREAM_FORMAT_ENCODED);
    pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_AC3);

    // Global Stereo Output Profile
    pAtts = g_AudioProfileStereo.output_descriptor.GetAttributes();
    pAtts->SetInt(MA_ATT_TYPE_STREAM_FORMAT,MA_STREAM_FORMAT_PCM);
    pAtts->SetInt(MA_ATT_TYPE_CHANNELS,2);
    pAtts->SetInt(MA_ATT_TYPE_BITDEPTH,16);
    pAtts->SetInt(MA_ATT_TYPE_SAMPLESPERSEC,48000);
    
    g_AudioProfileInit = true;
 }
 return &g_AudioProfileStereo;
}