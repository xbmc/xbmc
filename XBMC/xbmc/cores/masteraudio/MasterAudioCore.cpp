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
#include "MasterAudioCore.h"

// TODO: Move with mixer
#include "DirectSoundAdapter.h"

#include "Filters/AC3Encoder.h"

unsigned __int64 audio_slice_id = 0;

// CStreamInput
//////////////////////////////////////////////////////////////////////////////////////
CStreamInput::CStreamInput() : 
  m_pSlice(NULL)
{

}

CStreamInput::~CStreamInput()
{
  delete m_pSlice;
}

MA_RESULT CStreamInput::AddData(void* pBuffer, size_t bufLen)
{
  if (!pBuffer)
    return MA_ERROR;

  if (m_pSlice)
    return MA_BUSYORFULL;

  m_pSlice = audio_slice::create_slice(bufLen);

  memcpy(m_pSlice->get_data(), pBuffer, bufLen);

  return MA_SUCCESS;
}

IAudioSource* CStreamInput::GetSource()
{
  return (IAudioSource*)this;
}

void CStreamInput::Reset()
{
  delete m_pSlice;
  m_pSlice = NULL;
}

// IAudioSource
MA_RESULT CStreamInput::TestOutputFormat(CStreamDescriptor* pDesc)
{
  // Currently there is no format we cannot handle. All are treated as bitstreams
  return MA_SUCCESS;
}

MA_RESULT CStreamInput::SetOutputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Examine the stream descriptor
  // Currently all formats are treated as bitstreams. There is nothing to be done here.
  Reset();

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::GetOutputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps)
    return MA_ERROR;

  // We can do it all
  pProps->transfer_alignment = 0;
  pProps->max_transfer_size = 0;
  pProps->preferred_transfer_size = 0;

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::GetSlice(audio_slice** pSlice)
{
  if (!pSlice)
    return MA_ERROR;

  if (!m_pSlice)
    return MA_NEED_DATA;

  audio_slice* pOut = m_pSlice;
  m_pSlice = NULL;
  *pSlice = pOut;

  return MA_SUCCESS;
}

// CDSPChain
//////////////////////////////////////////////////////////////////////////////////////
CDSPChain::CDSPChain() :
  m_pInputSlice(NULL),
  m_pHead(NULL),
  m_pTail(NULL)
{

}

CDSPChain::~CDSPChain()
{
  Close();
}

IAudioSource* CDSPChain::GetSource()
{
  return (IAudioSource*)this;
}

IAudioSink* CDSPChain::GetSink()
{
  return (IAudioSink*)this;
}

MA_RESULT CDSPChain::CreateFilterGraph(CStreamDescriptor* pInDesc, CStreamDescriptor* pOutDesc)
{
  if (!pInDesc || !pOutDesc)
    return MA_ERROR;

  // See if the stream wants to be passed-through untouched (must be specified in both input AND output descriptors)
  m_Passthrough = false;
  if (MA_SUCCESS == pInDesc->GetAttributes()->GetBool(MA_ATT_TYPE_PASSTHROUGH,&m_Passthrough) && m_Passthrough)
  {
    m_Passthrough = false;
    if (MA_SUCCESS == pOutDesc->GetAttributes()->GetBool(MA_ATT_TYPE_PASSTHROUGH,&m_Passthrough) && m_Passthrough)
    {
      CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Detected passthrough stream. No filters will be created or applied.");
      return MA_SUCCESS;
    }
    else
    {
      CLog::Log(LOGWARNING,"MasterAudio:CDSPChain: Detected conflicting passthrough attributes. Passthrough will be ignored.");
    }
  }
 
  // TODO: Dynamically create DSP Filter Graph 
  /*
  // TEST: Encode all Streams to AC3
  IDSPFilter* pFilter = new CAC3Encoder();
  pFilter->SetInputFormat(pInDesc);
  pFilter->SetOutputFormat(pOutDesc);
  dsp_filter_node* pNode = new dsp_filter_node;
  pNode->filter = pFilter;
  pNode->next = NULL;
  pNode->prev = NULL;
  m_pHead = m_pTail = pNode;
*/
  CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Creating dummy filter graph.");

  return MA_SUCCESS;
}

// IDSPFilter
void CDSPChain::Close()
{
  DisposeGraph(); // Clean up any remaining filters
  delete m_pInputSlice; // This will not be going to anyone, so it must be cleaned-up
  m_pInputSlice = NULL;
}

// IAudioSource
MA_RESULT CDSPChain::TestOutputFormat(CStreamDescriptor* pDesc)
{
  return MA_ERROR; // This is currently only achievable through CreateFilterGraph()
}

MA_RESULT CDSPChain::SetOutputFormat(CStreamDescriptor* pDesc)
{
  return MA_ERROR; // This is currently only achievable through CreateFilterGraph()
}

MA_RESULT CDSPChain::GetOutputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps)
    return MA_ERROR;

  // Try to read properties from the last filter in the chain.
  if (m_pTail)
    return m_pTail->filter->GetOutputProperties(pProps);

  // Who knows what sizes we will send?
  // TODO: Is this a problem?
  pProps->max_transfer_size = 0;
  pProps->transfer_alignment = 0;
  pProps->preferred_transfer_size = 0;

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::GetSlice(audio_slice** ppSlice)
{
  // Currently just pass the input to the output
  if (!ppSlice)
    return MA_ERROR;

  if (m_pTail)
  {
    // Pull data through the DSP Filter Graph.
    return m_pTail->filter->GetSlice(ppSlice);
  }

  // Maybe we are just passing data through
  if (!m_pInputSlice)
  {
    *ppSlice = NULL;
    return MA_NEED_DATA;
  }

  *ppSlice = m_pInputSlice;
  m_pInputSlice = NULL; // Free up the input slot
  return MA_SUCCESS;
}

// IAudioSink
MA_RESULT CDSPChain::TestInputFormat(CStreamDescriptor* pDesc)
{
  return MA_ERROR; // This is currently only achievable through CreateFilterGraph(). Maybe we will implement dynamic configuration later.
}

MA_RESULT CDSPChain::SetInputFormat(CStreamDescriptor* pDesc)
{
  return MA_ERROR; // This is currently only achievable through CreateFilterGraph(). Maybe we will implement dynamic configuration later.
}

MA_RESULT CDSPChain::GetInputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps)
    return MA_ERROR;

  // Try to read properties from the first filter in the chain.
  if (m_pHead)
    return m_pHead->filter->GetInputProperties(pProps);

  // Accept anything
  pProps->max_transfer_size = 0;
  pProps->transfer_alignment = 0;
  pProps->preferred_transfer_size = 0;

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::AddSlice(audio_slice* pSlice)
{
  if (!pSlice)
    return MA_ERROR;

  // If we have a valid filter chain, push the data into it
  if (m_pHead)
    return m_pHead->filter->AddSlice(pSlice);

  // If we are performing passthrough, store this slice until it is requested on the output
  if (m_pInputSlice)
    return MA_BUSYORFULL; // Only one input slice can be queued at a time

  m_pInputSlice = pSlice;
  return MA_SUCCESS;
}

float CDSPChain::GetMaxLatency()
{
  // TODO: Calculate and return the actual latency through the filter graph
  return 0.0f;
}

void CDSPChain::Flush()
{
  // Flush each filter in the graph
  dsp_filter_node* pNode = m_pHead;
  while (pNode)
  {
    pNode->filter->Flush();
    pNode = pNode->next;
  }

  delete m_pInputSlice;
  m_pInputSlice = NULL; // We are the last ones with this data. Free it.
}

// Private Methods

void CDSPChain::DisposeGraph()
{
  // TODO: There must be a cleaner way to manage the nodes
  // Clean up all filters in the graph
  dsp_filter_node* pNode = m_pHead;
  while (pNode)
  {
    dsp_filter_node* pNext = pNode->next;
    pNode->filter->Close();
    delete pNode->filter;
    delete pNode;
    pNode = pNext;
  }
  m_pHead = NULL;
  m_pTail = NULL;
}

// CHardwareMixer
//////////////////////////////////////////////////////////////////////////////////////

CHardwareMixer::CHardwareMixer(int maxChannels) :
  m_MaxChannels(maxChannels),
  m_ActiveChannels(0),
  m_pChannel(NULL)
{
  // Create channel objects
  // TODO: Use channel factory
  m_pChannel = new IMixerChannel*[m_MaxChannels];
  for (int c=0;c<m_MaxChannels;c++)
    m_pChannel[c] = (IMixerChannel*)new CDirectSoundAdapter();
}

CHardwareMixer::~CHardwareMixer()
{
  // Clean-up channel objects
  if (m_pChannel)
  {
    for (int c=0;c<m_MaxChannels;c++)
      delete m_pChannel[c];
    delete[] m_pChannel;
  }
}

int CHardwareMixer::OpenChannel(CStreamDescriptor* pDesc)
{
  // Make sure we have a channel to open
  if (m_ActiveChannels >= m_MaxChannels)
  {
    CLog::Log(LOGWARNING,"MasterAudio:HardwareMixer: New channel requested, but none available (active_channels = %d, max_channels = %d).", m_ActiveChannels, m_MaxChannels);
    return 0;
  }  
  int channel = 0;

  // Find a free(idle) channel (there should be one since we are under max channels)
  for(int c = 0; c < m_MaxChannels; c++)
  {
    if (m_pChannel[c]->IsIdle())
    {
      channel = c + 1;
      m_pChannel[c]->SetInputFormat(pDesc);
      m_ActiveChannels++;
      break;
    }
  }
  CLog::Log(LOGINFO,"MasterAudio:HardwareMixer: Opened channel %d (active_channels = %d, max_channels = %d).", channel, m_ActiveChannels, m_MaxChannels);
  return channel;
}

void CHardwareMixer::CloseChannel(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return;

  CLog::Log(LOGINFO,"MasterAudio:HardwareMixer: Closing channel %d.", channel);

  m_pChannel[channel - 1]->Close();

  m_ActiveChannels--;
}

MA_RESULT CHardwareMixer::ControlChannel(int channel, int controlCode)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return MA_ERROR;

  int channelIndex = channel - 1;

  CLog::Log(LOGDEBUG,"MasterAudio:HardwareMixer: Control channel %d, code = %d", channel, controlCode);

  switch(controlCode)
  {
  case MA_CONTROL_STOP:
    m_pChannel[channelIndex]->Stop();
    break;
  case MA_CONTROL_PLAY:
    m_pChannel[channelIndex]->Play();
    break;
  case MA_CONTROL_PAUSE:
    m_pChannel[channelIndex]->Pause();
    break;
  case MA_CONTROL_RESUME:
    m_pChannel[channelIndex]->Resume();
    break;
  default:
    return MA_ERROR;
  }
  return MA_SUCCESS;
}

// TODO: Check channel state before controlling

MA_RESULT CHardwareMixer::SetChannelVolume(int channel, long vol)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return MA_ERROR;
  
  m_pChannel[channel-1]->SetVolume(vol);
  return MA_SUCCESS;
}

IAudioSink* CHardwareMixer::GetChannelSink(int channel)
{
  if (!channel || channel > m_MaxChannels)
    return NULL;

  return m_pChannel[channel-1];
}

float CHardwareMixer::GetMaxChannelLatency(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return 0.0f;  // Not going to render anything anyway
  
  return m_pChannel[channel-1]->GetMaxLatency();
}

void CHardwareMixer::FlushChannel(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return; // Nothing to flush
  
  m_pChannel[channel-1]->Flush();
}

bool CHardwareMixer::DrainChannel(int channel, unsigned int timeout)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return true; // Nothing to drain
  
  return m_pChannel[channel-1]->Drain(timeout);
}

// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue() :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0)
{

}

CSliceQueue::~CSliceQueue()
{
  // TODO: Cleanup any slices left in the queue
}

void CSliceQueue::Push(audio_slice* pSlice) 
{
  if (pSlice)
  {
    m_Slices.push(pSlice);
    m_TotalBytes += pSlice->header.data_len;
  }
}

audio_slice* CSliceQueue::Pop()
{
  audio_slice* pSlice = NULL;
  if (!m_Slices.empty())
  {
    pSlice = m_Slices.front();
    m_Slices.pop();
    m_TotalBytes -= pSlice->header.data_len;
  }
  return pSlice;
}

// TODO: Try to respect preferred transfer size
audio_slice* CSliceQueue::GetSlice(size_t alignSize, size_t maxSize)
{
  if (GetTotalBytes() < alignSize) // Need to have enough data to give
    return NULL;

  size_t bytesUsed = 0;
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  size_t sliceSize = std::min<size_t>(GetTotalBytes(),maxSize); // Set the size of the slice to be created, limit it to maxSize

  // If we have no remainder and the first whole slice can satisfy the requirements, pass it along as-is.
  // This reduces allocs and writes
  if (!m_RemainderSize) 
  {
    size_t headLen = m_Slices.front()->header.data_len;
    if (!(headLen % alignSize) && headLen <= maxSize)
    {
      pNext = Pop();
      return pNext;
    }
  }

  sliceSize -= sliceSize % alignSize; // Make sure we are properly aligned.
  audio_slice* pOutSlice = audio_slice::create_slice(sliceSize); // Create a new slice
  
  // See if we can fill the new slice out of our partial slice (if there is one)
  if (m_RemainderSize >= sliceSize)
  {
    memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , sliceSize);
    m_RemainderSize -= sliceSize;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }

    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved
    {
      pNext = Pop();
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > sliceSize) // Check for a partial slice
        remainder = nextLen - (sliceSize - bytesUsed);
      memcpy(pOutSlice->get_data() + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        delete pNext; // Free the copied slice
    } while (bytesUsed < sliceSize);
  }

  // Clean up the previous partial slice
  if (!m_RemainderSize)
  {
    delete m_pPartialSlice;
    m_pPartialSlice = NULL;
  }

  // Save off a new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }

  return pOutSlice;
}

// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CAudioDataInterconnect::CAudioDataInterconnect() :
  m_pSource(NULL),
  m_pSink(NULL),
  m_pSlice(NULL)
{

}

CAudioDataInterconnect::~CAudioDataInterconnect()
{

}

MA_RESULT CAudioDataInterconnect::Link(IAudioSource* pSource, IAudioSink* pSink)
{
  if (!pSource || !pSink)
    return MA_ERROR; // No connection possible

  // Store interface pointers
  m_pSource = pSource;
  m_pSink = pSink;

  m_pSource->GetOutputProperties(&m_InputProps);
  if (m_InputProps.max_transfer_size == 0) m_InputProps.max_transfer_size = 0xFFFFFFFF; // TODO: Define a global max
  if (m_InputProps.transfer_alignment == 0) m_InputProps.transfer_alignment = 1; // TODO: Define a global max
  m_pSink->GetInputProperties(&m_OutputProps);
  if (m_OutputProps.max_transfer_size == 0) m_OutputProps.max_transfer_size = 0xFFFFFFFF; // TODO: Define a global max
  if (m_OutputProps.transfer_alignment == 0) m_OutputProps.transfer_alignment = 1; // TODO: Define a global max

  return MA_SUCCESS;
}

void CAudioDataInterconnect::Unlink()
{
  m_pSource = NULL;
  m_pSink = NULL;
}

MA_RESULT CAudioDataInterconnect::Process()
{
  // Check for a valid connection
  if (!m_pSource || !m_pSink)
    return MA_ERROR; // No connection

  // Decide if we want to accept more input from the Source. If so, get some.
  if (m_Queue.GetTotalBytes() < m_OutputProps.transfer_alignment) // We want at least 1 'transfer unit' of data
  {
    audio_slice* pSlice = NULL;
    if (MA_SUCCESS == m_pSource->GetSlice(&pSlice))
      m_Queue.Push(pSlice);
  }

  // See if we have data to send to the sink. If not, try to find some
  if (!m_pSlice)
    m_pSlice = m_Queue.GetSlice(m_OutputProps.transfer_alignment, m_OutputProps.max_transfer_size); // TODO: Try to respect preferred_transfer_size

  // If we have something for the sink, send it along
  if (m_pSlice)
  {
    if (MA_SUCCESS == m_pSink->AddSlice(m_pSlice))
      m_pSlice = NULL; // We are done with it
  }

  return MA_SUCCESS; // TODO: We can probably return a more meaningful result here
}

void CAudioDataInterconnect::Flush()
{
  if (m_pSink)
    m_pSink->Flush(); // Flush downstream sink

  // Empty any buffers
}

size_t CAudioDataInterconnect::GetCacheSize()
{
  size_t size = m_Queue.GetTotalBytes();
  if (m_pSlice)
    size += m_pSlice->header.data_len;
  return 0;
}


// CStreamAttributeCollection
//////////////////////////////////////////////////////////////////////////////////////
CStreamAttributeCollection::CStreamAttributeCollection()
{

}

CStreamAttributeCollection::~CStreamAttributeCollection()
{

}

MA_RESULT CStreamAttributeCollection::GetInt(MA_ATTRIB_ID id, int* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_int)
    return MA_TYPE_MISMATCH;

  *pVal = pAtt->intVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetFloat(MA_ATTRIB_ID id, float* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_float)
    return MA_TYPE_MISMATCH;

  *pVal = pAtt->floatVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetString(MA_ATTRIB_ID id, char** pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_string)
    return MA_TYPE_MISMATCH;

  *pVal = pAtt->stringVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetPtr(MA_ATTRIB_ID id, void** pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_ptr)
    return MA_TYPE_MISMATCH;

  *pVal = pAtt->ptrVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetBool(MA_ATTRIB_ID id, bool* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_bool)
    return MA_TYPE_MISMATCH;

  *pVal = pAtt->boolVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetInt(MA_ATTRIB_ID id, int val)
{
  stream_attribute att;
  att.intVal = val;
  att.type = stream_attribute_int;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetFloat(MA_ATTRIB_ID id, float val)
{
  stream_attribute att;
  att.floatVal = val;
  att.type = stream_attribute_float;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetString(MA_ATTRIB_ID id, char* val)
{
  stream_attribute att;
  att.stringVal = val;
  att.type = stream_attribute_string;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetPtr(MA_ATTRIB_ID id, void* val)
{
  stream_attribute att;
  att.ptrVal = val;
  att.type = stream_attribute_ptr;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}
MA_RESULT CStreamAttributeCollection::SetBool(MA_ATTRIB_ID id, bool val)
{
  stream_attribute att;
  att.boolVal = val;
  att.type = stream_attribute_bool;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}

stream_attribute* CStreamAttributeCollection::FindAttribute(MA_ATTRIB_ID id)
{
  StreamAttributeIterator iter = m_Attributes.find(id);
  if (iter != m_Attributes.end())
    return &iter->second;
  return NULL;
}