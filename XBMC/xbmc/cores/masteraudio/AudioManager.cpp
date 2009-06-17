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
#include "Settings.h"

// Global (ick) Instance
CAudioManager g_AudioLibManager;

// CAudioStreamFactory
//////////////////////////////////////////////////////////////////////////////////
CAudioStreamFactory::CAudioStreamFactory(IAudioMixer* pMixer) :
  m_pMixer(pMixer)
{

}

CAudioStreamFactory::~CAudioStreamFactory()
{

}

CAudioStream* CAudioStreamFactory::Create(CStreamDescriptor* pInDesc)
{
  // This is all about identifying the output format. Input format is fixed.
  CStreamDescriptor outputDesc = *pInDesc; // Start with a copy of the input descriptor
  CStreamAttributeCollection* pOutAttribs = outputDesc.GetAttributes();

  // For MA_STREAM_FORMAT_IEC61937, we can only pass-through
  // For MA_STREAM_FORMAT_LPCM, we have a number of explicit changes. 
  // We update the output format, and then see if the Transform Chain can handle it 
  // TODO: Probe configuration for user-defined transforms for the given input format.
  //  Define sets of transforms as modes or features
  // TODO: Need a database-type format query mechanism.
  if (pInDesc->GetFormat() == MA_STREAM_FORMAT_LPCM)
  {
    bool ac3Encode = g_guiSettings.GetBool("audiooutput.ac3encode");
    bool upmix = g_guiSettings.GetBool("audiooutput.upmix");
    bool downmix = g_guiSettings.GetBool("audiooutput.downmixmultichannel");

    // Resampling changes sample rate and may affect available channels, so handle it first
    if (g_advancedSettings.m_musicResample && !ac3Encode)
      pOutAttribs->SetUInt(MA_ATT_TYPE_SAMPLERATE, g_advancedSettings.m_musicResample);

    // TODO: How many channels shoud we mix to?
    unsigned int maxChannels = 8;
    unsigned int bytesPerSample = 0;
    if (MA_SUCCESS != pInDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_BITDEPTH, &bytesPerSample))
      return NULL;

    bytesPerSample /= 8;

    // Downmixing decreases channel count
    // TODO: How many channels shoud we mix to?
    if (downmix) // Downmix takes precedence over upmix
    {
      int channelLayout[2] = {MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT};
      pOutAttribs->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT, 2);
      pOutAttribs->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, bytesPerSample * 2);
      pOutAttribs->SetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, 2, channelLayout);
    }
    // Upmixing increases channel count
    // TODO: How many channels shoud we mix to?
    else if (upmix)
    {
      pOutAttribs->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT, maxChannels);
      pOutAttribs->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, bytesPerSample * maxChannels);
    }

    // Encoding changes the output format
    if (ac3Encode)
    {
      // TODO: This only supports 48KHz
      outputDesc.SetFormat(MA_STREAM_FORMAT_IEC61937);
      pOutAttribs->SetUInt(MA_ATT_TYPE_ENCODING, MA_STREAM_ENCODING_AC3);
      pOutAttribs->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, 6144);
      pOutAttribs->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC, 192000);
      pOutAttribs->SetUInt(MA_ATT_TYPE_SAMPLERATE, 48000);
    }
  }

  // Open a mixer channel using the pre-constructed output format
  // TODO: At the moment, if it is not supported, we simply fail
  IRenderingAdapter* pRenderAdapter = m_pMixer->OpenChannel(&outputDesc);  // Virtual Mixer Channel
  if (!pRenderAdapter)
    return NULL;

  CDSPChain* pChain = new CDSPChain();
  // Attempt to create a compound Transform using the identified formats
  if (MA_SUCCESS != pChain->CreateFilterGraph(pInDesc, &outputDesc))
  {
    m_pMixer->CloseChannel(pRenderAdapter);
    delete pChain;
    return NULL;
  }

  // We should be good to go. Wrap everything up in a new Stream object
  CStreamInput* pInput = new CStreamInput();
  CAudioStream* pStream = new CAudioStream();
  if ((MA_SUCCESS != pInput->SetOutputFormat(pInDesc)) || (MA_SUCCESS != pStream->Initialize(pInput, pChain, pRenderAdapter)))
  {
    m_pMixer->CloseChannel(pRenderAdapter);
    delete pChain;
    delete pInput;
    delete pStream;
    return NULL;
  }

  return pStream;
}

///////////////////////////////////////////////////////////////////////////////
// CAudioManager
///////////////////////////////////////////////////////////////////////////////
CAudioManager::CAudioManager() :
  m_pMixer(NULL),
  m_MaxStreams(MA_MAX_INPUT_STREAMS),
  m_Initialized(false)
{

}

CAudioManager::~CAudioManager()
{
  m_ProcessThread.StopThread();

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

  // Clean up sync event pool
  while (m_SyncHandlePool.size())
  {
    CloseHandle(m_SyncHandlePool.top());
    m_SyncHandlePool.pop();
  }
}

// TODO: This is not thread-safe, but will be OK until final integration
bool CAudioManager::Initialize()
{
  if (!m_Initialized)
  {
    m_ProcessThread.Create(this);
    m_Initialized = true;
  }
  return m_Initialized;
}

bool CAudioManager::SetMixerType(int mixerType)
{
  delete m_pMixer;
  delete m_pStreamFactory;

  m_pMixer = NULL;
  m_pStreamFactory = NULL;

  switch(mixerType)
  {
  case MA_MIXER_HARDWARE:
    m_pMixer = new CHardwareMixer(m_MaxStreams);
    break;
  }

  if (m_pMixer)
    m_pStreamFactory = new CAudioStreamFactory(m_pMixer);

  return (m_pMixer != NULL);
}

// Thread-Safe Client Interface
//////////////////////////////////////////////////////////////////////////////////////////
MA_STREAM_ID CAudioManager::OpenStream(CStreamDescriptor* pDesc)
{
  if (!m_Initialized)
    Initialize();

  // Find out if we can handle an additional input stream, if not, give up
  if (!pDesc || m_StreamList.size() >= m_MaxStreams)
  {
    CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream. Too many active streams. (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
    return MA_STREAM_NONE;
  }

  if (!m_pMixer)
    SetMixerType(MA_MIXER_HARDWARE);

  CAudioStream* pStream = m_pStreamFactory->Create(pDesc);
    
  // Call into the service thread to add the new stream to our collection
  MA_STREAM_ID streamId = GetStreamId(pStream);
  CAudioManagerMessagePtr msg(streamId, CAudioManagerMessage::ADD_STREAM, pStream);
  if (!CallMethodSync(&msg) || !msg.GetSyncData()->m_SyncResult.boolVal)
  {
    delete pStream;
    CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
    return MA_STREAM_NONE;
  }
  
  CLog::Log(LOGINFO,"MasterAudio:AudioManager: Opened stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), m_MaxStreams);
  return streamId;
}

size_t CAudioManager::AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len)
{
  CAddStreamDataMessage msg(streamId, pData, len);  
  if (CallMethodSync(&msg))
    return msg.GetSyncData()->m_SyncResult.uint32Val;

  return 0;
}

void CAudioManager::ControlStream(MA_STREAM_ID streamId, int controlCode)
{
  CTransportControlMessage* pMsg = new CTransportControlMessage(streamId, controlCode);
  CallMethodAsync(pMsg);
}

void CAudioManager::SetStreamVolume(MA_STREAM_ID streamId, long vol)
{
  CAudioManagerMessageLong* pMsg = new CAudioManagerMessageLong(streamId, CAudioManagerMessage::SET_STREAM_VOLUME, vol);
  CallMethodAsync(pMsg);
}

float CAudioManager::GetStreamDelay(MA_STREAM_ID streamId)
{
  CAudioManagerMessage msg(streamId, CAudioManagerMessage::GET_STREAM_DELAY);  
  if (CallMethodSync(&msg))
    return msg.GetSyncData()->m_SyncResult.floatVal;

  return 0.0f;
}

bool CAudioManager::DrainStream(MA_STREAM_ID streamId, unsigned int timeout)
{
  CAudioManagerMessageLong msg(streamId, CAudioManagerMessage::DRAIN_STREAM, timeout);

  if (CallMethodSync(&msg))
    return msg.GetSyncData()->m_SyncResult.boolVal;

  return false;
}

void CAudioManager::FlushStream(MA_STREAM_ID streamId)
{
  CAudioManagerMessage* pMsg = new CAudioManagerMessage(streamId, CAudioManagerMessage::FLUSH_STREAM);  
  CallMethodAsync(pMsg);
}

void CAudioManager::CloseStream(MA_STREAM_ID streamId)
{
  CAudioManagerMessage* pMsg = new CAudioManagerMessage(streamId, CAudioManagerMessage::CLOSE_STREAM);  
  CallMethodAsync(pMsg);
}

bool CAudioManager::SetStreamProp(MA_STREAM_ID streamId, int propId, const void* pVal)
{
  // TODO: Implement
  return false;
}

bool CAudioManager::GetStreamProp(MA_STREAM_ID streamId, int propId, void* pVal)
{
  // TODO: Implement
  return false;
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
  m_pMixer->CloseChannel(pStream->GetRenderingAdapter());
  pStream->Destroy();
}

// Message Queue-Based Procedure call mechanism 
void CAudioManager::PostMessage(CAudioManagerMessage* pMsg)
{
  m_Queue.PushBack(pMsg);
  m_ProcessThread.Wake();
}

void CAudioManager::CallMethodAsync(CAudioManagerMessage* pMsg)
{
  // This just posts a message to the queue and returns
  return PostMessage(pMsg);
}

bool CAudioManager::CallMethodSync(CAudioManagerMessage* pMsg)
{
  CSyncCallData syncCall;

  // We need a wait event to sync with the processing thread on the other side of the 'wall'
  if (!m_SyncHandlePool.size())
    syncCall.m_SyncWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  else
  {
    CSingleLock lock(m_SyncPoolLock);
    syncCall.m_SyncWaitEvent = m_SyncHandlePool.top();
    m_SyncHandlePool.pop();
    lock.Leave();
    ResetEvent(syncCall.m_SyncWaitEvent);
  }

  syncCall.m_SyncResult.int64Val = 0; // This should set all members to 0
  pMsg->SetSyncData(&syncCall); // Associate the thread's sync event with the current message

  // Post the message to the queue. This will signal the processing thread.
  PostMessage(pMsg);

  // Wait for the message to be processed
  // TODO: Allow the client to specify a timeout? 
  WaitForSingleObject(syncCall.m_SyncWaitEvent, INFINITE);

  // Return the sync event to the pool
  CSingleLock lock(m_SyncPoolLock);
  m_SyncHandlePool.push(syncCall.m_SyncWaitEvent);

  return true;
}

// AudioManager Processing Thread
///////////////////////////////////////////////////////////////////////////////

// External interface
CAudioManager::CAudioManagerThread::CAudioManagerThread() : 
  CThread()
{
  m_ProcessEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CAudioManager::CAudioManagerThread::~CAudioManagerThread()
{
  CloseHandle(m_ProcessEvent);
}

void CAudioManager::CAudioManagerThread::Wake()
{
  // Poke the thread and force it to run if it is not already
  SetEvent(m_ProcessEvent);
}

void CAudioManager::CAudioManagerThread::Create(CAudioManager* pMgr)
{
  CLog::Log(LOGDEBUG, "%s: Creating Audio Service Processing Thread...", __FUNCTION__);
  m_pManager = pMgr;
  CThread::Create();
}

void CAudioManager::CAudioManagerThread::Process()
{
  CLog::Log(LOGDEBUG, "%s: Audio Service Processing Thread Started. ThreadId = %d", __FUNCTION__, ThreadId());

  try 
  {
    // Message handling and processing loop
    while (!m_bStop)
    {
      // Process pending client requests
      CAudioManagerMessage* pMsg = NULL;
      while ((pMsg = m_pManager->m_Queue.PopFront()) != NULL)
      {
        if (pMsg->IsAsync())
          HandleMethodAsync(pMsg);
        else
          HandleMethodSync(pMsg);
      }

      // Perform primary processing
      RenderStreams();

      ResetEvent(m_ProcessEvent);
      // If there are no more messages, wait to be signaled or time-out.
      if (m_pManager->GetOpenStreamCount())
        WaitForSingleObject(m_ProcessEvent, 5);
      else // Wait for someone to wake us if there are no streams to process
        WaitForSingleObject(m_ProcessEvent, INFINITE);
    }
  }
  catch (...)
  {
    CLog::Log(LOGDEBUG, "%s: There was an exception in the Audio Service Processing Thread.", __FUNCTION__);
  }
  CLog::Log(LOGDEBUG, "%s: Audio Service Processing Thread Stopped.", __FUNCTION__);
}

void CAudioManager::CAudioManagerThread::RenderStreams()
{
  // TODO: This needs to be more efficient. Possibly stopping if processing takes too long
  if (m_pManager->m_StreamList.size())
  {
    for (StreamIterator iter = m_pManager->m_StreamList.begin();iter != m_pManager->m_StreamList.end();iter++)
    {
      CAudioStream* pStream = (CAudioStream*)iter->second;
      pStream->Render();    
    }
  }
}

void CAudioManager::CAudioManagerThread::HandleMethodAsync(CAudioManagerMessage* pMsg)
{
  // Dispatch an incoming asynchronous procedure-call message to the appropriate handler
  MA_STREAM_ID streamId = pMsg->GetStreamId();
  switch(pMsg->GetClass())
  {
  case CAudioManagerMessage::CONTROL_STREAM:
    ControlStream(streamId,((CTransportControlMessage*)pMsg)->GetCommand());
    break;
  case CAudioManagerMessage::FLUSH_STREAM:
    FlushStream(streamId);
    break;
  case CAudioManagerMessage::SET_STREAM_VOLUME:
    SetStreamVolume(streamId, ((CAudioManagerMessageLong*)pMsg)->GetParam());
    break;
  case CAudioManagerMessage::CLOSE_STREAM:
    CloseStream(streamId);
    break;
  default:
    CLog::Log(LOGDEBUG, "%s: Caller provided an invalid MessageClass.", __FUNCTION__);
    break;
  }
  delete pMsg;
}

bool CAudioManager::CAudioManagerThread::HandleMethodSync(CAudioManagerMessage* pMsg)
{
  // Dispatch an incoming synchronous procedure-call message to the appropriate handler,
  // store the result, and notify the waiting caller
  MA_STREAM_ID streamId = pMsg->GetStreamId();
  CSyncCallData* pSync = pMsg->GetSyncData();
  switch(pMsg->GetClass())
  {
  case CAudioManagerMessage::ADD_STREAM:
    pSync->m_SyncResult.boolVal = AddStream(streamId, (CAudioStream*)((CAudioManagerMessagePtr*)pMsg)->GetParam());
    break;
  case CAudioManagerMessage::ADD_DATA:
    pSync->m_SyncResult.uint32Val = AddDataToStream(streamId, ((CAddStreamDataMessage*)pMsg)->GetData(), ((CAddStreamDataMessage*)pMsg)->GetDataLen());
    break;
  case CAudioManagerMessage::GET_STREAM_DELAY:
    pSync->m_SyncResult.floatVal = GetStreamDelay(streamId);
    break;
  case CAudioManagerMessage::DRAIN_STREAM:
    DrainStream(streamId, (int)((CAudioManagerMessageLong*)pMsg)->GetParam());
    break;
  default:
    CLog::Log(LOGDEBUG, "%s: Caller provided an invalid MessageClass.", __FUNCTION__);
    return false;
  }

  SetEvent(pSync->m_SyncWaitEvent); // Signal that the call is complete
  return true;
}

// Procedure Call Handlers
////////////////////////////////////////////////
bool CAudioManager::CAudioManagerThread::AddStream(MA_STREAM_ID streamId, CAudioStream* pStream)
{
  // Add the new stream to our collection
  m_pManager->m_StreamList[streamId] = pStream;

  return true;
}

void CAudioManager::CAudioManagerThread::ControlStream(MA_STREAM_ID streamId, int command)
{
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if(pStream)
    pStream->SendCommand(command);
}

void CAudioManager::CAudioManagerThread::SetStreamVolume(MA_STREAM_ID streamId, long volume)
{
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if(pStream)
    pStream->SetLevel(((float)volume + 6000.0f)/6000.0f);
}

float CAudioManager::CAudioManagerThread::GetStreamDelay(MA_STREAM_ID streamId)
{
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if (!pStream)
    return 0.0f;

  return pStream->GetDelay();
}

unsigned int CAudioManager::CAudioManagerThread::AddDataToStream(MA_STREAM_ID streamId, void* pData, unsigned int len)
{
  unsigned int bytesAdded = 0;

  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if (!pStream)
    return 0;

  // Add the data to the stream
  if(MA_SUCCESS == pStream->AddData(pData,len))
    bytesAdded = len;
  else
    bytesAdded = 0;

  return bytesAdded;
}

bool CAudioManager::CAudioManagerThread::DrainStream(MA_STREAM_ID streamId, int timeout)
{
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if (!pStream)
    return true; // Nothing to drain

  return pStream->Drain(timeout);
}

void CAudioManager::CAudioManagerThread::FlushStream(MA_STREAM_ID streamId)
{
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if (pStream)
    pStream->Flush();
}

void CAudioManager::CAudioManagerThread::CloseStream(MA_STREAM_ID streamId)
{
  StreamIterator iter = m_pManager->m_StreamList.find(streamId);
  if (iter == m_pManager->m_StreamList.end())
    return;
  CAudioStream* pStream = iter->second;
  m_pManager->m_StreamList.erase(iter);
  m_pManager->CleanupStreamResources(pStream);
  delete pStream;

  CLog::Log(LOGINFO,"MasterAudio:AudioManager: Closed stream %d (active_stream = %d, max_streams = %d).", streamId, m_pManager->GetOpenStreamCount(), m_pManager->m_MaxStreams);
}