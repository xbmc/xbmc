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

unsigned __int64 audio_slice_id = 0;

// CStreamInput
//////////////////////////////////////////////////////////////////////////////////////
CStreamInput::CStreamInput() : 
  m_OutputSize(0),
  m_pBuffer(NULL),
  m_BufferOffset(0)
{

}

CStreamInput::~CStreamInput()
{
  delete m_pBuffer;
}

MA_RESULT CStreamInput::Initialize()
{
  // TODO: Should we be setting a default output buffer size?
  // TODO: What else needs to be done here?
  if (m_OutputSize)
    ConfigureBuffer();

  return MA_SUCCESS;
}

// TODO: Consider adding logic to back-down the size of the buffer over time (if it is large).
MA_RESULT CStreamInput::AddData(void* pBuffer, size_t bufLen)
{
  // Process a block of data from a client.  We attempt to be as flexible as possible, but a few rules still apply:
  //  1. We only ever work on one block of data at a time, as long as it is bigger than the output size
  //  2. The internal buffer will grow to accomodate any input block size, but will not concatenate 2 or more blocks (except remainders)

  // Take a hint from the client about the size of things to come
  if (!m_OutputSize)
    m_OutputSize = bufLen;  //TODO: This really should be set externally or put in the descriptor

  if (!m_pBuffer)
    ConfigureBuffer();

  if (bufLen > m_pBuffer->GetMaxLen()) // Take this as a hint that the client wants to send bigger chunks and increase our buffer size
  {
    // Take this as a hint that the client wants to write bigger blocks (we do not want to do this too often)
    CLog::Log(LOGINFO,"MasterAudio:CStreamInput: Increasing buffer size (current_size = %d, new_size = %d).", m_pBuffer->GetMaxLen(), bufLen);
    size_t newSize = bufLen;
    if (m_pBuffer->GetLen() < m_OutputSize)
      newSize = bufLen * 2; // Misalignment causes us to require more space (TODO: how much space do we really need?) 

    CSimpleBuffer* pNewBuffer = new CSimpleBuffer();
    pNewBuffer->Initialize(newSize);

    // Copy remaining data from our old buffer
    pNewBuffer->Write(m_pBuffer->GetData(NULL),m_pBuffer->GetLen());

    // Make the switch
    delete m_pBuffer;
    m_pBuffer = pNewBuffer;
  }

  // See if we already have an output slice in the buffer. If we do, reject the caller this time around.
  //  We shoud be able to take their data next time around. If we do not have enough already, continue on.
  if (m_pBuffer->GetLen() >= m_OutputSize)
    return MA_BUSYORFULL;

  if (!m_pBuffer->Write(pBuffer, bufLen))// Add this input block to the buffer (accumulator)
    return MA_ERROR;

  return MA_SUCCESS;
}

IAudioSource* CStreamInput::GetSource()
{
  return (IAudioSource*)this;
}

void CStreamInput::SetOutputSize(size_t size)
{
  // TODO: Make sure this doesn't leave us short on input
  m_OutputSize = size;
  ConfigureBuffer();
}

void CStreamInput::Reset()
{
  // TODO: is there anything else to be done here? Maybe resize/delete the buffer?
  m_pBuffer->Empty();
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
  // TODO: Should we clear/reset the buffer on a format change, assuming that the data is not compatible?

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::GetSlice(audio_slice** pSlice)
{
  if (!pSlice || !m_pBuffer)
    return MA_ERROR;

  audio_slice* pOut = NULL;
  *pSlice = NULL; // Default return value

  // TODO: Implement slice allocator and reduce number of memcpy's

  size_t bufferLen = m_pBuffer->GetLen();
  if ((bufferLen - m_BufferOffset) >= m_OutputSize)  // If we have enough data in our buffer, carve off a slice
  {
    // Create a new slice object and copy the output data into it
    pOut = audio_slice::create_slice(m_OutputSize);
    unsigned char* pBlock = (unsigned char*)m_pBuffer->GetData(NULL);
    memcpy(pOut->get_data(), &pBlock[m_BufferOffset], m_OutputSize);

    // Update our internal offset. We use this so we don't have to shift the data for each read (performance improvement)
    m_BufferOffset += m_OutputSize;
    size_t bytesLeft = bufferLen - m_BufferOffset;
    if (!bytesLeft) // No more data. Reset the buffer.
    {
      // TODO: This would be the best time to shrink the buffer if we wanted to do so.
      m_pBuffer->Empty();
      m_BufferOffset = 0;
    }
    else if (bytesLeft < m_OutputSize)  // Move any stragglers to the top of the buffer (performance hit)
    {
      m_pBuffer->ShiftUp(m_pBuffer->GetLen() - bytesLeft);
      m_BufferOffset = 0;
    }
    *pSlice = pOut; // Must be freed by someone else
  }
  else
    return MA_NEED_DATA;

  return MA_SUCCESS;
}

void CStreamInput::ConfigureBuffer()
{
  if (!m_pBuffer)
    m_pBuffer = new CSimpleBuffer();
  // TODO: See if we can recover any data from the buffer. Currently we just wipe the buffer.
  m_pBuffer->Initialize(m_OutputSize);
  m_BufferOffset = 0;
}

// CDSPChain
//////////////////////////////////////////////////////////////////////////////////////
CDSPChain::CDSPChain() :
  m_pInputSlice(NULL)
{

}

CDSPChain::~CDSPChain()
{
  // Clean up any remaining filters
  DisposeGraph();
  delete m_pInputSlice; // This will not be going to anyone, so it must be cleaned-up
}

IAudioSource* CDSPChain::GetSource()
{
  return (IAudioSource*)this;
}

IAudioSink* CDSPChain::GetSink()
{
  return (IAudioSink*)this;
}

MA_RESULT CDSPChain::CreateFilterGraph(CStreamDescriptor* pDesc)
{
  if (!pDesc)
    return MA_ERROR;

  // See if stream wants to be passed-through untouched
  bool passthrough = false;
  if (MA_SUCCESS == pDesc->GetAttributes()->GetBool(MA_ATT_TYPE_PASSTHROUGH,&passthrough) && passthrough)
  {
    CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Detected passthrough stream. No filters will be created or applied.");
    return MA_SUCCESS;
  }
  
  // TODO: Dynamically create DSP Filter Graph 
  CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Creating dummy filter graph.");
  return MA_SUCCESS;
}

// IAudioSource
MA_RESULT CDSPChain::TestOutputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::SetOutputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::GetSlice(audio_slice** pSlice)
{
  // Currently just pass the input to the output
  // TODO: Push data through the DSP Filter Graph.
  if (!pSlice)
    return MA_ERROR;

  if (!m_pInputSlice)
  {
    *pSlice = NULL;
    return MA_NEED_DATA;
  }

  *pSlice = m_pInputSlice;

  m_pInputSlice = NULL; // Free up the input slot

  return MA_SUCCESS;
}

// IAudioSink
MA_RESULT CDSPChain::TestInputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::SetInputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CDSPChain::AddSlice(audio_slice* pSlice)
{
  if (!pSlice)
    return MA_ERROR;

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
  // TODO: Flush filter graph

  delete m_pInputSlice;
  m_pInputSlice = NULL; // We are the last ones with this data. Free it.
}

// Private Methods

void CDSPChain::DisposeGraph()
{
  // Clean up all filters in the graph
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
    CLog::Log(LOGWARNING,"MasterAudio:PassthroughMixer: New channel requested, but none available (active_channels = %d, max_channels = %d).", m_ActiveChannels, m_MaxChannels);
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
  CLog::Log(LOGINFO,"MasterAudio:PassthroughMixer: Opened channel %d (active_channels = %d, max_channels = %d).", channel, m_ActiveChannels, m_MaxChannels);
  return channel;
}

void CHardwareMixer::CloseChannel(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return;

  CLog::Log(LOGINFO,"MasterAudio:PassthroughMixer: Closing channel %d.", channel);

  m_pChannel[channel - 1]->Close();

  m_ActiveChannels--;
}

MA_RESULT CHardwareMixer::ControlChannel(int channel, int controlCode)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return MA_ERROR;

  int channelIndex = channel - 1;

  CLog::Log(LOGDEBUG,"MasterAudio:PassthroughMixer: Control channel %d, code = %d", channel, controlCode);

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