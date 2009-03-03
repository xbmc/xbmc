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
  m_pMixerSink(NULL), // DO NOT delete the sink!
  m_pInputSourceSlice(NULL),
  m_pDSPSourceSlice(NULL)
{

}

CAudioStream::~CAudioStream()
{
  // Delete any in-process slices (we cannot use them and cannot pass them on)
  delete m_pInputSourceSlice;
  delete m_pDSPSourceSlice;
}

bool CAudioStream::Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, int mixerChannel, IAudioSink* pMixerSink)
{
  // TODO: Move more responsibility from AudioManager::OpenStream
  m_pInput = pInput;
  m_pDSPChain = pDSPChain;
  m_MixerChannel = mixerChannel;
  m_pMixerSink = pMixerSink;
  return true;
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

// Private Methods
////////////////////////////////////////////////////////////////////////////////
bool CAudioStream::ProcessStream()
{
  bool ret = true;
  MA_RESULT res = MA_SUCCESS;

  // 1. If we don't have one already, pull slice from CStreamInput
  if (!m_pInputSourceSlice)
  {
    if (MA_ERROR == m_pInput->GetSlice(&m_pInputSourceSlice))
      ret = false;
  }

  // 2. If we have one to give, pass a slice to CDSPChain sink
  if(m_pInputSourceSlice)
  {
    res = m_pDSPChain->GetSink()->AddSlice(m_pInputSourceSlice);
    if (MA_ERROR == res)
      ret = false;
    else if (MA_SUCCESS == res)
      m_pInputSourceSlice = NULL; // We are done with this one
  }
 
  // 3. If we don't have one already, pull slice from CDSPChain source
  if (!m_pDSPSourceSlice)
  {
    if (MA_ERROR == m_pDSPChain->GetSource()->GetSlice(&m_pDSPSourceSlice))
      ret = false;
  }

  // 4. If we have one to give, pass a slice to the output channel
  if (m_pDSPSourceSlice)
  {
    res = m_pMixerSink->AddSlice(m_pDSPSourceSlice);
    if (MA_ERROR == res)
      ret = false;
    else if (MA_SUCCESS == res)
      m_pDSPSourceSlice = NULL; // We are done with this one
  }
  return ret;
}

bool CAudioStream::NeedsData()
{
  // TODO: Implement properly
  return false; 
}

size_t CAudioStream::GetCacheSize()
{
  // TODO: Implement properly
  return 0;
}

unsigned int CAudioStream::GetCacheTime()
{
  // TODO: Implement properly
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// CAudioManager
///////////////////////////////////////////////////////////////////////////////
CAudioManager::CAudioManager() :
  m_pMixer(NULL)
{

}

CAudioManager::~CAudioManager()
{
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

MA_STREAM_ID CAudioManager::OpenStream(CStreamDescriptor* pDesc, size_t blockSize)
{
  // 0. Find out if we can handle an additional input stream, if not, give up
  if(m_StreamList.size() >= MA_MAX_INPUT_STREAMS)
    return MA_STREAM_NONE;
  
  // 0.5 Create an input handler for this stream
  CStreamInput* pInput = new CStreamInput(blockSize);
  MA_RESULT res = MA_ERROR;
  res = pInput->Initialize(pDesc);
  if (MA_SUCCESS != res)
  {
    delete pInput;
    return MA_STREAM_NONE;
  }

  // 1. See if this stream type is configured to pass-through to the mixer untouched, if it is make it so
  // TODO: Implement

  // 2. See if we can build a processing chain to handle this stream, if not, try and pass it through to the mixer or give up
  CDSPChain* pChain = new CDSPChain();
  if (MA_SUCCESS != pChain->CreateFilterGraph(pDesc))
  {
    delete pInput;
    delete pChain;
    return MA_STREAM_NONE;
  }

  // 3. Create a new channel in the mixer to handle the stream
  if (!m_pMixer)
    SetMixerType(MA_MIXER_HARDWARE);
  int mixerChannel = m_pMixer->OpenChannel(pDesc);
  IAudioSink* pMixerSink = m_pMixer->GetChannelSink(mixerChannel);
  if (!mixerChannel || !pMixerSink)
  {
    delete pInput;
    delete pChain;
    return MA_STREAM_NONE;
  }

  // 4. Wire everything up
  CAudioStream* pStream = new CAudioStream();
  pStream->Initialize(pInput, pChain, mixerChannel, pMixerSink);  

  if (!AddInputStream(pStream))
  {
    delete pStream; // Should clean everything else up
    return MA_STREAM_NONE;
  }

  MA_STREAM_ID streamId = GetStreamId(pStream);

  CLog::Log(LOGINFO,"MasterAudio:AudioManager: Opened stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), MA_MAX_INPUT_STREAMS);

  return streamId;
}

size_t CAudioManager::AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len)
{
  size_t bytesRead = 0;

  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return 0;

  CStreamInput* pInput = pStream->GetInput();
  if (!pInput)
    return 0;

  if(MA_SUCCESS == pInput->AddData(pData,len))
  {
    bytesRead = len;
    //CLog::Log(LOGDEBUG,"^^ MASTER_AUDIO:AudioManager - StreamInput ACCEPTED %d bytes of input", len);
  }
  else
  {
    bytesRead = 0;
    //CLog::Log(LOGDEBUG,"** MASTER_AUDIO:AudioManager - StreamInput REJECTED %d bytes of input", len);
  }

  // Push data through the stream
  pStream->ProcessStream();

  return bytesRead;
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

float CAudioManager::GetStreamDelay(MA_STREAM_ID streamId)
{
  // TODO: Implement
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return 0.0f;

  return 0.0f;
}

void CAudioManager::DrainStream(MA_STREAM_ID streamId, unsigned int maxTime)
{
  // TODO: Implement
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return;
}

void CAudioManager::FlushStream(MA_STREAM_ID streamId)
{
  // TODO: Implement
  CAudioStream* pStream = GetInputStream(streamId);
  if (!pStream)
    return;
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

  CLog::Log(LOGINFO,"MasterAudio:AudioManager: Closed stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), MA_MAX_INPUT_STREAMS);
}

bool CAudioManager::SetMixerType(int mixerType)
{
  if (m_pMixer)
    delete m_pMixer;

  m_pMixer = NULL;

  switch(mixerType)
  {
  case MA_MIXER_HARDWARE:
    m_pMixer = new CPassthroughMixer(MA_MAX_INPUT_STREAMS);
    return true;
  case MA_MIXER_SOFTWARE:
  default:
    return false;
  }
}

/////////////////////////////////////////////////////////////////////////////////////
// Private Methods
/////////////////////////////////////////////////////////////////////////////////////
bool CAudioManager::AddInputStream(CAudioStream* pStream)
{
  // Limit the maximum number of open streams
  if(m_StreamList.size() >= MA_MAX_INPUT_STREAMS)
    return false;

  MA_STREAM_ID id = GetStreamId(pStream);
  m_StreamList[id] = pStream;
  return true;
}

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
}
