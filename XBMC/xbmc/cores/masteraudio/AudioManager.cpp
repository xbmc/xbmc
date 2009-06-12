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


///////////////////////////////////////////////////////////////////////////////
// CAudioProfile
///////////////////////////////////////////////////////////////////////////////
CAudioProfile::CAudioProfile()
{

}

CAudioProfile::~CAudioProfile()
{
  Clear();
}

void CAudioProfile::Clear()
{
  for (StreamDescriptorList::iterator iter = m_DescriptorList.begin(); iter != m_DescriptorList.end(); iter++)
    delete *iter; 
  m_DescriptorList.clear();
}

unsigned int CAudioProfile::GetDescriptorCount()
{
  return m_DescriptorList.size();
}

CStreamDescriptor* CAudioProfile::GetDescriptor(unsigned int index)
{
  return m_DescriptorList[index];
}

bool CAudioProfile::GetDescriptors(int format, StreamDescriptorList* pList)
{
  if (!pList)
    return false;

  pList->clear();

  // TODO: A map is probably better for this, but the list will likely be short anyway
  for (StreamDescriptorList::iterator iter = m_DescriptorList.begin(); iter != m_DescriptorList.end(); iter++)
  {
    if ((*iter)->GetFormat() == format)
      pList->push_back(*iter);
  }
  return (pList->size() > 0);
}

void CAudioProfile::AddDescriptor(CStreamDescriptor* pDesc)
{
  m_DescriptorList.push_back(pDesc);
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

// Thread-Safe Client Interface
//////////////////////////////////////////////////////////////////////////////////////////
MA_STREAM_ID CAudioManager::OpenStream(CStreamDescriptor* pDesc)
{
  if (!m_Initialized)
    Initialize();

  // TODO: Move to global initialization. Keep here for now to pick-up changes to settings
  LoadAudioProfiles();

  // Find out if we can handle an additional input stream, if not, give up
  if (!pDesc || m_StreamList.size() >= m_MaxStreams)
  {
    CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream. Too many active streams. (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
    return MA_STREAM_NONE;
  }

  // TODO: RenderingAdapter should be provided TO the mixer, not come FROM it...
  // Create components for a new AudioStream
  CAudioStream* pStream = new CAudioStream();
  CStreamInput* pInput = new CStreamInput();        // Input Handler
  CDSPChain* pChain = new CDSPChain();              // DSP Handling Chain
  if (!m_pMixer)
    SetMixerType(MA_MIXER_HARDWARE);
  IRenderingAdapter* renderAdapter = NULL;
  while (m_pMixer)
  {
    // No need to test the StreamInput output format, as it treats all data the same. Provide it with the descriptor anyway.
    if (MA_SUCCESS != pInput->SetOutputFormat(pDesc))
      break;

    StreamDescriptorList outputDescriptors;
    if (!FindOutputDescriptors(pDesc, &outputDescriptors))
      break; // We don't know what to do with this stream

    CStreamDescriptor* pOutputDesc = NULL;
    for (StreamDescriptorList::iterator iter = outputDescriptors.begin(); iter != outputDescriptors.end(); iter++)
    {
      // Open a mixer channel using the preferred format
      renderAdapter = m_pMixer->OpenChannel(*iter);  // Virtual Mixer Channel
      if (!renderAdapter)
        continue; // TODO: Keep trying until a working format is found or we run out of options

      // Attempt to create the DSP Filter Graph using the identified formats (TODO: Can this be updated on the fly?)
      if (MA_SUCCESS != pChain->CreateFilterGraph(pDesc, *iter))
        continue; // We cannot convert from the input format to the output format

      pOutputDesc = *iter;
      break;
    }

    if (!pOutputDesc)
      break; // We were not able to find a usable output descriptor

    // We should be good to go. Wrap everything up in a new Stream object
    if (MA_SUCCESS != pStream->Initialize(pInput, pChain, renderAdapter))
      break;
    
    // Call into the service thread to add the new stream to our collection
    MA_STREAM_ID streamId = GetStreamId(pStream);
    CAudioManagerMessagePtr msg(streamId, CAudioManagerMessage::ADD_STREAM, pStream);
    if (!CallMethodSync(&msg) || !msg.GetSyncData()->m_SyncResult.boolVal)
      break;

    CLog::Log(LOGINFO,"MasterAudio:AudioManager: Opened stream %d (active_stream = %d, max_streams = %d).", streamId, GetOpenStreamCount(), m_MaxStreams);
    return streamId;
  }

  // Something went wrong.  Clean up and return.
  pStream->Destroy();
  if (m_pMixer)
    m_pMixer->CloseChannel(pStream->GetRenderingAdapter());
  delete pStream;
  delete pChain;
  delete pInput;
  
  CLog::Log(LOGERROR,"MasterAudio:AudioManager: Unable to open new stream (active_stream = %d, max_streams = %d).", GetOpenStreamCount(), m_MaxStreams);
  return MA_STREAM_NONE;
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

// Audio Profiles are used to define specific audio scenarios based on user configuration, available hardware, and source stream characteristics
// Returns a prioritized list of descriptors
bool CAudioManager::FindOutputDescriptors(CStreamDescriptor* pInputDesc, StreamDescriptorList* pList)
{
  // TODO: Need to find a more flexible/intelligent way to handle this for N possible input/output formats
  // Preference, unless overridden, is an exact match to the input descriptor

  if (!pInputDesc)
    return false;

  StreamDescriptorList matchList;
  pList->clear();
  int inputFormat = pInputDesc->GetFormat();

  // So-Called 'passthrough' formats. Currently we can only pass them along to the output
  if (inputFormat == MA_STREAM_FORMAT_IEC61937)
  {
    // Fetch any output descriptors that support this format
    if (m_DefaultProfile.GetDescriptors(inputFormat, &matchList))
    {
      // Find one that has the correct encoding and sample rate
      int inputEncoding = MA_STREAM_ENCODING_UNKNOWN;
      unsigned int inputSampleRate = 0;
      pInputDesc->GetAttributes()->GetInt(MA_ATT_TYPE_ENCODING, &inputEncoding);
      pInputDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &inputSampleRate);
      for (StreamDescriptorList::iterator iter = matchList.begin(); iter != matchList.end(); iter++)
      {
        CStreamDescriptor* pOutDesc = *iter;
        int outputEncoding = MA_STREAM_ENCODING_UNKNOWN;
        unsigned int outputSampleRate = 0;
        pOutDesc->GetAttributes()->GetInt(MA_ATT_TYPE_ENCODING, &outputEncoding);
        pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &outputSampleRate);
        if (inputEncoding == outputEncoding && inputSampleRate == outputSampleRate)
        {
          pList->push_back(pOutDesc);
          return true; // We only need one, since there will be no processing performed
        }
      }
    }
    return false; // Nothing we can do. The output does not support this format, and we can't convert it.
  }

  if (inputFormat == MA_STREAM_FORMAT_LPCM)
  {
    bool ac3Encode = g_guiSettings.GetBool("audiooutput.ac3encode");
    bool upmix = g_guiSettings.GetBool("audiooutput.upmix");
    bool downmix = g_guiSettings.GetBool("audiooutput.downmixmultichannel");

    unsigned int inputSampleRate = 0;
    unsigned int inputChannels = 0;
    pInputDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &inputSampleRate);
    pInputDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &inputChannels);

    // Check for explicit transforms
    if (ac3Encode && (inputChannels > 2 || upmix)) // Use the AC3 encoder only if there are greater than 2 input channels or we are upmixing
    {
        // Try and find an output descriptor that is compatible with the ac3 encoder
      if (m_DefaultProfile.GetDescriptors(MA_STREAM_FORMAT_IEC61937, &matchList))
      {
        for (StreamDescriptorList::iterator iter = matchList.begin(); iter != matchList.end(); iter++)
        {
          CStreamDescriptor* pOutDesc = *iter;
          int outputEncoding = MA_STREAM_ENCODING_UNKNOWN;
          unsigned int outputSampleRate = 0;
          pOutDesc->GetAttributes()->GetInt(MA_ATT_TYPE_ENCODING, &outputEncoding);
          pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &outputSampleRate);
          if (outputEncoding == MA_STREAM_ENCODING_AC3 && inputSampleRate == outputSampleRate)
            pList->push_back(pOutDesc);
        }
      }
    }

    if (m_DefaultProfile.GetDescriptors(inputFormat, &matchList))
    {
      unsigned int maxChannels = 0;
      CStreamDescriptor* pMaxOut = NULL;
      CStreamDescriptor* pMatchingOut = NULL;
      for (StreamDescriptorList::iterator iter = matchList.begin(); iter != matchList.end(); iter++)
      {
        CStreamDescriptor* pOutDesc = *iter;
        unsigned int outputChannels = 0;
        unsigned int outputSampleRate = 0;
        pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &outputChannels);
        pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &outputSampleRate);
        if (outputSampleRate == inputSampleRate)
        {
          if (outputChannels > maxChannels)
          {
            maxChannels = outputChannels;
            pMaxOut = pOutDesc;
          }
          if (outputChannels == inputChannels)
            pMatchingOut = pOutDesc; // Save for later so we add in the right order
        }
      }
      if (upmix && pMaxOut) // If we are upmixing, add the matching output with the most channels
        pList->push_back(pMaxOut);
      if (pMatchingOut)
        pList->push_back(pMatchingOut); // TODO: Add case to create a matching descriptor using the max_out descriptor (for 3 or 4 channel streams etc...)
      else if (!upmix)
        pList->push_back(pMaxOut); // Fall back to whatever we have for now

    }
  }
  
  return (pList->size() > 0);
}

void CAudioManager::LoadAudioProfiles()
{
  // TODO: Implement user configuration and multiple profiles

  m_DefaultProfile.Clear();

  // Load descriptors into the default profile based on guisettings
  CStreamDescriptor* pDesc;
  CStreamAttributeCollection* pAtts;
  bool downMix = g_guiSettings.GetBool("audiooutput.downmixmultichannel");
  bool ac3Enabled = g_guiSettings.GetBool("audiooutput.ac3passthrough");
  bool dtsEnabled = g_guiSettings.GetBool("audiooutput.dtspassthrough");

  // 2-channel PCM @ 48kHz, 16-bit Signed Int, Interleaved
  pDesc = new CStreamDescriptor();
  pDesc->SetFormat(MA_STREAM_FORMAT_LPCM);
  pAtts = pDesc->GetAttributes();
  pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,false);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,4);
  pAtts->SetFlag(MA_ATT_TYPE_LPCM_FLAGS,MA_LPCM_FLAG_INTERLEAVED,true);
  pAtts->SetInt(MA_ATT_TYPE_SAMPLE_TYPE,MA_SAMPLE_TYPE_SINT);
  pAtts->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT,2);
  pAtts->SetUInt(MA_ATT_TYPE_BITDEPTH,16);
  pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,48000);
  m_DefaultProfile.AddDescriptor(pDesc);

   // 2-channel PCM @ 44.1kHz, 16-bit Signed Int, Interleaved
  pDesc = new CStreamDescriptor();
  pDesc->SetFormat(MA_STREAM_FORMAT_LPCM);
  pAtts = pDesc->GetAttributes();
  pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,false);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,176400);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,4);
  pAtts->SetFlag(MA_ATT_TYPE_LPCM_FLAGS,MA_LPCM_FLAG_INTERLEAVED,true);
  pAtts->SetInt(MA_ATT_TYPE_SAMPLE_TYPE,MA_SAMPLE_TYPE_SINT);
  pAtts->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT,2);
  pAtts->SetUInt(MA_ATT_TYPE_BITDEPTH,16);
  pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,44100);
  m_DefaultProfile.AddDescriptor(pDesc);

  if (!downMix) // Use this as a way for the user to say that they do not support > 2 channel PCM
  {
    // 6-Channel PCM @ 48kHz, 16-bit Signed Int, Interleaved
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_LPCM);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,false);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,576000);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,12);
    pAtts->SetFlag(MA_ATT_TYPE_LPCM_FLAGS,MA_LPCM_FLAG_INTERLEAVED,true);
    pAtts->SetInt(MA_ATT_TYPE_SAMPLE_TYPE,MA_SAMPLE_TYPE_SINT);
    pAtts->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT,6);
    pAtts->SetUInt(MA_ATT_TYPE_BITDEPTH,16);
    pAtts->SetInt(MA_ATT_TYPE_SAMPLERATE,48000);
    m_DefaultProfile.AddDescriptor(pDesc);

    // 6-Channel PCM @ 44.1kHz, 16-bit Signed Int, Interleaved
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_LPCM);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,false);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,529200);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,12);
    pAtts->SetFlag(MA_ATT_TYPE_LPCM_FLAGS,MA_LPCM_FLAG_INTERLEAVED,true);
    pAtts->SetInt(MA_ATT_TYPE_SAMPLE_TYPE,MA_SAMPLE_TYPE_SINT);
    pAtts->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT,6);
    pAtts->SetUInt(MA_ATT_TYPE_BITDEPTH,16);
    pAtts->SetInt(MA_ATT_TYPE_SAMPLERATE,44100);
    m_DefaultProfile.AddDescriptor(pDesc);
  }

  if (ac3Enabled)
  {
    // AC3 @ 48kHz
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_IEC61937);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,true);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
    pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_AC3);
    pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,48000);
    m_DefaultProfile.AddDescriptor(pDesc);

    // AC3 @ 44.1kHz
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_IEC61937);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,true);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
    pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_AC3);
    pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,44100);
    m_DefaultProfile.AddDescriptor(pDesc);
  }

  if (dtsEnabled)
  {
    // DTS @ 48kHz
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_IEC61937);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,true);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
    pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_DTS);
    pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,48000);
    m_DefaultProfile.AddDescriptor(pDesc);

    // DTS @ 44.1kHz
    pDesc = new CStreamDescriptor();
    pDesc->SetFormat(MA_STREAM_FORMAT_IEC61937);
    pAtts = pDesc->GetAttributes();
    pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,true);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,176400);
    pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
    pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_DTS);
    pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,44100);
    m_DefaultProfile.AddDescriptor(pDesc);
  }
}

// Message Queue-Based Procedure call mechanism 
__declspec (thread) CSyncCallData t_AudioClient;

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
  // We need a wait event to sync with the processing thread on the other side of the 'wall'
  // This is kept in thread-local storage for each calling thread
  // TODO: !!Need a way to ensure cleanup of events when client threads exit!!
  if (!t_AudioClient.m_SyncWaitEvent)
    t_AudioClient.m_SyncWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  else
    ResetEvent(t_AudioClient.m_SyncWaitEvent);

  t_AudioClient.m_SyncResult.int64Val = 0;
  pMsg->SetSyncData(&t_AudioClient); // Associate the thread's sync event with the current message

  // Post the message to the queue. This will signal the processing thread.
  PostMessage(pMsg);

  // Wait for the message to be processed
  // TODO: Allow the client to specify a timeout? 
  WaitForSingleObject(t_AudioClient.m_SyncWaitEvent, INFINITE);

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
  PulseEvent(m_ProcessEvent);
}

void CAudioManager::CAudioManagerThread::Create(CAudioManager* pMgr)
{
  CLog::Log(LOGDEBUG, "%s: Creating Audio Service Processing Thread...", __FUNCTION__);
  m_pManager = pMgr;
  CThread::Create();
}

void CAudioManager::CAudioManagerThread::Process()
{
  CLog::Log(LOGDEBUG, "%s: Audio Service Processing Thread Started.", __FUNCTION__);

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

      // If there are no more messages, wait to be signaled or time-out.
      WaitForSingleObject(m_ProcessEvent, 20);
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
  // TODO: There must be a cleaner way to do this (why can't the AudioStream drain the mixer channel?)
  CAudioStream* pStream = m_pManager->GetInputStream(streamId);
  if (!pStream)
    return true; // Nothing to drain

  lap_timer timer;
  timer.lap_start();
  if (pStream->Drain(timeout))
  {
    unsigned __int64 elapsed = timer.elapsed_time()/1000;
    if (elapsed < timeout)
      return pStream->Drain(timeout - (unsigned int)elapsed);
    else
      pStream->Flush();
  }
  return false; // Out of time
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