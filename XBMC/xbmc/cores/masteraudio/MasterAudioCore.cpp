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
CStreamInput::CStreamInput(size_t inputBlockSize) : 
  m_InputBlockSize(inputBlockSize),
  m_OutputSize(inputBlockSize),
  m_BufferOffset(0)
{

}

CStreamInput::~CStreamInput()
{

}

MA_RESULT CStreamInput::Initialize(CStreamDescriptor* pDesc)
{
  // TODO: Examine the stream descriptor
  ConfigureBuffer();

  return MA_SUCCESS;
}

bool CStreamInput::CanAcceptData()
{
  if (m_InputBlockSize >= m_OutputSize)  // We are splitting up input slices or m_InputBlickSize == m_OutputSize
  {
    if (m_Buffer.GetLen())
      return false; // We are still processing the last block
  }
  else // We need more than one input block for each output slice
  {
    if (m_Buffer.GetSpace() < m_InputBlockSize)
      return false; // We are still processing the last full slice (no more room in the buffer), or input and output are not aligned
  }
  return true;
}

MA_RESULT CStreamInput::AddData(void* pBuffer, size_t bufLen)
{
  if (bufLen != m_InputBlockSize)
    return MA_ERROR;  // TODO: Not sure how to handle this.  The caller is misbehaving. We may be able to let it slide sometimes

  if (bufLen >= m_OutputSize)  // We will be splitting this up into multiple slices (or just one, but the handling is the same)
  {
    if (m_Buffer.GetLen())
      return MA_BUSYORFULL; // We are still processing the last block

    // Save the input block so we can split it up and/or pass it along
    m_Buffer.Write(pBuffer, bufLen);
  }
  else // We need more than one input block for each output slice
  {
    if (m_Buffer.GetSpace() < bufLen)
      return MA_BUSYORFULL; // We are still processing the last full slice (no more room in the buffer), or input and output are not aligned

    // Add this input block to the buffer (accumulator)
    m_Buffer.Write(pBuffer, bufLen);
  }

  return MA_SUCCESS;
}

IAudioSource* CStreamInput::GetSource()
{
  return (IAudioSource*)this;
}

void CStreamInput::SetInputBlockSize(size_t size)
{
  m_InputBlockSize = size;
  ConfigureBuffer();
}

void CStreamInput::SetOutputSize(size_t size)
{
  m_OutputSize = size;
  ConfigureBuffer();
}

// IAudioSource
MA_RESULT CStreamInput::TestOutputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::SetOutputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement Properly

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::GetSlice(audio_slice** pSlice)
{
  if (!pSlice)
    return MA_ERROR;

  audio_slice* pOut = NULL;
  *pSlice = NULL;

  // See if we have any data to give
  if (!m_Buffer.GetLen())
    return MA_BUSYORFULL;

  // TODO: Implement slice allocator and reduce number of memcpy's

  if (m_InputBlockSize >= m_OutputSize)  // Carve a slice from the buffer
  {
    if(m_InputBlockSize - m_BufferOffset < m_OutputSize)
      return MA_ERROR;  // mis-aligned input and output sizes ( our buffer is not big enough to handle this ) TODO: Some math in CofigureBuffer should remedy this

    pOut = audio_slice::create_slice(m_OutputSize);
    unsigned char* pBlock = (unsigned char*)m_Buffer.GetData(NULL);
    memcpy(pOut->get_data(), &pBlock[m_BufferOffset], m_OutputSize);
    m_BufferOffset += m_OutputSize;
    if(m_BufferOffset >= m_InputBlockSize)
    {
      m_Buffer.Empty();
      m_BufferOffset = 0;
    }
    *pSlice = pOut; // Must be freed by someone else
  }
  else // it takes multiple inputs to make an output, see if we have a complete output
  {
    if (m_Buffer.GetLen() < m_OutputSize)
      return MA_NEED_DATA;

    pOut = audio_slice::create_slice(m_OutputSize);
    memcpy(pOut->get_data(), m_Buffer.GetData(NULL), m_OutputSize);

    // Move any left over data up in the buffer. 
    // This only happens when our output size is not a multiple of out input size
    // and degrades performance (but at least we can handle it)
    unsigned int bytesLeft = m_Buffer.GetLen() - m_OutputSize;
    if (bytesLeft)
      m_Buffer.ShiftUp(bytesLeft);
    else
      m_Buffer.Empty();

    *pSlice = pOut; // Must be freed by someone else
  }
  //CLog::Log(LOGDEBUG,"<--MASTER_AUDIO:StreamInput - sourcing slice %I64d, len=%d", pOut->header.id,pOut->header.data_len);

  return MA_SUCCESS;
}

void CStreamInput::ConfigureBuffer()
{
  // TODO: if m_InputBlockSize and m_OutputSize are not multiples of one another we will have problems
  size_t bufferSize = std::max<size_t>(m_InputBlockSize,m_OutputSize);
  // TODO: See if we can recover any data from the buffer. Currently we just wipe the buffer.
  m_Buffer.Initialize(bufferSize);
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
  // This will not be going to anyone, so it must be cleaned-up
  delete m_pInputSlice;
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
  // TODO: Implement Properly

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
  if (!pSlice)
    return MA_ERROR;

  if (!m_pInputSlice)
  {
    *pSlice = NULL;
    return MA_NEED_DATA;
  }

  //CLog::Log(LOGDEBUG,"<----MASTER_AUDIO:DSPChain - sourcing slice %I64d, len=%d", m_pInputSlice->header.id, m_pInputSlice->header.data_len);

  *pSlice = m_pInputSlice;

  m_pInputSlice = NULL;

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
    return MA_BUSYORFULL;

  //CLog::Log(LOGDEBUG,"<----MASTER_AUDIO:DSPChain - sinking slice %I64d, len=%d", pSlice->header.id, pSlice->header.data_len);

  m_pInputSlice = pSlice;;
  return MA_SUCCESS;
}

void CDSPChain::DisposeGraph()
{

}

// CPassthroughMixer
//////////////////////////////////////////////////////////////////////////////////////

CPassthroughMixer::CPassthroughMixer(int maxChannels) :
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

CPassthroughMixer::~CPassthroughMixer()
{
  // Clean-up channel objects
  if (m_pChannel)
  {
    for (int c=0;c<m_MaxChannels;c++)
      delete m_pChannel[c];
    delete[] m_pChannel;
  }
}

int CPassthroughMixer::OpenChannel(CStreamDescriptor* pDesc)
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

void CPassthroughMixer::CloseChannel(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return;

  CLog::Log(LOGINFO,"MasterAudio:PassthroughMixer: Closing channel %d.", channel);

  m_pChannel[channel - 1]->Close();

  m_ActiveChannels--;
}

MA_RESULT CPassthroughMixer::ControlChannel(int channel, int controlCode)
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

MA_RESULT CPassthroughMixer::SetChannelVolume(int channel, long vol)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return MA_ERROR;
  
  m_pChannel[channel-1]->SetVolume(vol);
  return MA_SUCCESS;
}

IAudioSink* CPassthroughMixer::GetChannelSink(int channel)
{
  if (!channel || channel > m_MaxChannels)
    return NULL;

  return m_pChannel[channel-1];
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

stream_attribute* CStreamAttributeCollection::FindAttribute(MA_ATTRIB_ID id)
{
  StreamAttributeIterator iter = m_Attributes.find(id);
  if (iter != m_Attributes.end())
    return &iter->second;
  return NULL;
}