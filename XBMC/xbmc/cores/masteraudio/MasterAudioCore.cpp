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
#include "Filters/DSPFilter.h"

unsigned __int64 audio_slice_id = 0;

ma_audio_container* ma_alloc_container(unsigned int buffers, size_t bytesPerFrame, size_t framesPerBuffer)
{
  ma_audio_container* pCont = (ma_audio_container*)calloc(1, sizeof(ma_audio_container) + sizeof(ma_audio_buffer) * (buffers - 1));
  pCont->buffer_count = buffers;
  size_t bytesPerBuffer = bytesPerFrame * framesPerBuffer;
  unsigned char* data = (unsigned char*)calloc(1, bytesPerBuffer * buffers);
  for(unsigned int i = 0; i < buffers; i++)
  {
    pCont->buffer[i].data = data + (bytesPerBuffer * i);
    pCont->buffer[i].allocated_len = bytesPerBuffer;
  }
  return pCont;
}

void ma_free_container(ma_audio_container* pCont)
{
  if (pCont)
  {
    free(pCont->buffer[0].data);
    free(pCont);
  }
}

// CStreamInput
//////////////////////////////////////////////////////////////////////////////////////
CStreamInput::CStreamInput()
{
}

CStreamInput::~CStreamInput()
{
}

MA_RESULT CStreamInput::AddData(void* pBuffer, size_t bufLen)
{
  if (!pBuffer)
    return MA_ERROR;

  if ((size_t)m_Cache.GetMaxWriteSize() >= bufLen)
  {  
    m_Cache.WriteBinary((const char*)pBuffer, bufLen);
    return MA_SUCCESS;
  }

  return MA_BUSYORFULL;
}

void CStreamInput::Reset()
{

}

// IAudioSource
MA_RESULT CStreamInput::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if(!pDesc)
    return MA_ERROR;
  
  if (bus > 0) // Only one bus
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  // Verify that the required attributes are present and of the correct type
  if (
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS, MA_STREAM_FLAG_LOCKED, NULL) != MA_SUCCESS) ||
    (pAttribs->GetUInt(MA_ATT_TYPE_BYTES_PER_SEC, NULL) != MA_SUCCESS) ||
    (pAttribs->GetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT, NULL) != MA_SUCCESS)) 
    return MA_MISSING_ATTRIBUTE;

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if(!pDesc)
    return MA_ERROR;
  
  if (bus > 0) // Only one bus
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  if ((pAttribs->GetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, &m_BytesPerFrame) != MA_SUCCESS) ||
      (pAttribs->GetUInt(MA_ATT_TYPE_BYTES_PER_SEC, &m_BytesPerSecond) != MA_SUCCESS))
     return MA_MISSING_ATTRIBUTE;

  m_Cache.Create(m_BytesPerSecond);

  return MA_SUCCESS;
}

MA_RESULT CStreamInput::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  unsigned int bytesToRender = (m_BytesPerFrame * frameCount);
  if ((unsigned int)m_Cache.GetMaxReadSize() >= bytesToRender)
  {
    if (pOutput->buffer[0].allocated_len < bytesToRender)
      return MA_BUF_TOO_SMALL;
    if (m_Cache.ReadBinary((char*)pOutput->buffer[0].data, bytesToRender))
    {
      pOutput->buffer[0].data_len = bytesToRender;
      return MA_SUCCESS;
    }
    return MA_ERROR;
  }
  return MA_NEED_DATA;
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

// TODO: Improve this method's usefulness 
void CHardwareMixer::Render(int channel)
{
  if (!channel || !m_ActiveChannels || channel > m_MaxChannels)
    return;

  m_pChannel[channel-1]->Render();
}

// CStreamAttributeCollection
//////////////////////////////////////////////////////////////////////////////////////
CStreamAttributeCollection::CStreamAttributeCollection()
{

}

CStreamAttributeCollection::~CStreamAttributeCollection()
{
  // TODO: Free BLOB and Array data

}

MA_RESULT CStreamAttributeCollection::GetInt(MA_ATTRIB_ID id, int* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_int)
    return MA_TYPE_MISMATCH;

  if (pVal)
    *pVal = pAtt->intVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetInt64(MA_ATTRIB_ID id, __int64* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_int64)
    return MA_TYPE_MISMATCH;

  if (pVal)
    *pVal = pAtt->int64Val;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetUInt(MA_ATTRIB_ID id, unsigned int* pVal)
{
  return GetInt(id, (int*)pVal);
}

MA_RESULT CStreamAttributeCollection::GetUInt64(MA_ATTRIB_ID id, unsigned __int64* pVal)
{
  return GetInt64(id, (__int64*)pVal);
}

MA_RESULT CStreamAttributeCollection::GetFloat(MA_ATTRIB_ID id, float* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;

  if (pAtt->type != stream_attribute_float)
    return MA_TYPE_MISMATCH;

  if (pVal)
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

  if (pVal)
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

  if (pVal)
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

  if (pVal)
    *pVal = pAtt->boolVal;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetFlag(MA_ATTRIB_ID id, int flag, bool* pVal)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;
  
  if (pAtt->type != stream_attribute_bitfield)
    return MA_TYPE_MISMATCH;

  if (pVal)
    *pVal = (pAtt->bitfieldVal & flag) ? true : false;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetBlob(MA_ATTRIB_ID id, void* pBuf, unsigned int bufLen)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;
  
  if (pAtt->type != stream_attribute_blob)
    return MA_TYPE_MISMATCH;

  if (pBuf)
  {
    if (bufLen < pAtt->dataLen)
      return MA_BUF_TOO_SMALL;
    memcpy(pBuf, pAtt->ptrVal, pAtt->dataLen);
  }

  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetBlobLen(MA_ATTRIB_ID id, unsigned int* pLen)
{
  if (!pLen)
    return MA_ERROR;

  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;
  
  if (pAtt->type != stream_attribute_blob)
    return MA_TYPE_MISMATCH;

  *pLen = pAtt->dataLen;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetArray(MA_ATTRIB_ID id, int elementType, void* pBuf, unsigned int bufLen)
{
  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;
  
  int attType = pAtt->type ^ stream_attribute_array;
  if (attType != elementType || !(pAtt->type & stream_attribute_array) || !pAtt->dataLen)
    return MA_TYPE_MISMATCH;

  if (!pBuf)
    return MA_SUCCESS;

  unsigned int elementSize = 0;
  switch(attType)
  {
  case stream_attribute_bitfield:
  case stream_attribute_int:
    elementSize = sizeof(int);
    break;
  case stream_attribute_int64:
    elementSize = sizeof(__int64);
    break;
  case stream_attribute_float:
    elementSize = sizeof(float);
    break;
  case stream_attribute_bool:
    elementSize = sizeof(bool);
    break;
  case stream_attribute_ptr:
  case stream_attribute_string:
  case stream_attribute_blob:
    elementSize = sizeof(void*);
  default:
    return MA_TYPE_MISMATCH;
  }
  if (bufLen < pAtt->dataLen)
    return MA_BUF_TOO_SMALL;

  memcpy(pBuf, pAtt->ptrVal, pAtt->dataLen);
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::GetArraySize(MA_ATTRIB_ID id, stream_attribute_type elementType, unsigned int* pSize)
{
  if (!pSize)
    return MA_ERROR;

  stream_attribute* pAtt = FindAttribute(id);
  if (!pAtt)
    return MA_NOTFOUND;
  
  int attType = pAtt->type ^ stream_attribute_array;
  if (attType != elementType || !(pAtt->type & stream_attribute_array) || !pAtt->dataLen)
    return MA_TYPE_MISMATCH;

  unsigned int elementSize = 0;
  switch(attType)
  {
  case stream_attribute_bitfield:
  case stream_attribute_int:
    elementSize = sizeof(int);
    break;
  case stream_attribute_int64:
    elementSize = sizeof(__int64);
    break;
  case stream_attribute_float:
    elementSize = sizeof(float);
    break;
  case stream_attribute_bool:
    elementSize = sizeof(bool);
    break;
  case stream_attribute_ptr:
  case stream_attribute_string:
  case stream_attribute_blob:
    elementSize = sizeof(void*);
  default:
    return MA_TYPE_MISMATCH;
  }
  
  *pSize = pAtt->dataLen * elementSize;
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

MA_RESULT CStreamAttributeCollection::SetInt64(MA_ATTRIB_ID id, __int64 val)
{
  stream_attribute att;
  att.int64Val = val;
  att.type = stream_attribute_int64;
  m_Attributes[id] = att;
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetUInt(MA_ATTRIB_ID id, unsigned int val)
{
  return SetInt(id, (int)val);
}

MA_RESULT CStreamAttributeCollection::SetUInt64(MA_ATTRIB_ID id, unsigned __int64 val)
{
  return SetInt64(id, (__int64)val);
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

MA_RESULT CStreamAttributeCollection::SetFlag(MA_ATTRIB_ID id, int flag, bool val)
{
  stream_attribute att;
  stream_attribute* pAtt = FindAttribute(id);
  if (pAtt)
  {
    if (pAtt->type != stream_attribute_bitfield)
      return MA_TYPE_MISMATCH;
    att = *pAtt;
  }
  else
    att.type = stream_attribute_bitfield;

  if (val)
   att.bitfieldVal |= flag;
  else
    att.bitfieldVal ^= flag;
  m_Attributes[id] = att;
  
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetBlob(MA_ATTRIB_ID id, void* pBuf, unsigned int bufLen)
{
  if (!pBuf || !bufLen)
    return MA_ERROR;

  stream_attribute* pAtt = FindAttribute(id);
  if (pAtt) // Already exists. 
  {
    if (pAtt->type != stream_attribute_blob)
      return MA_TYPE_MISMATCH;
    if (pAtt->dataLen != bufLen) // If sizes don't match, need to release data from previous value.
    {
      free(pAtt->ptrVal);
      pAtt->ptrVal = malloc(bufLen);
      pAtt->dataLen = bufLen;
    }
    memcpy(pAtt->ptrVal, pBuf, bufLen);
  }
  else
  {
    stream_attribute att;
    att.type = stream_attribute_blob;
    att.ptrVal = malloc(bufLen);
    att.dataLen = bufLen;
    memcpy(att.ptrVal, pBuf, bufLen);
    m_Attributes[id] = att;
  }
  return MA_SUCCESS;
}

MA_RESULT CStreamAttributeCollection::SetArray(MA_ATTRIB_ID id, stream_attribute_type elementType, unsigned int elementCount, void* pElements)
{
  if (!pElements || !elementCount)
    return MA_ERROR;

  unsigned int elementSize = 0;
  switch(elementType)
  {
  case stream_attribute_bitfield:
  case stream_attribute_int:
    elementSize = sizeof(int);
    break;
  case stream_attribute_int64:
    elementSize = sizeof(__int64);
    break;
  case stream_attribute_float:
    elementSize = sizeof(float);
    break;
  case stream_attribute_bool:
    elementSize = sizeof(bool);
    break;
  case stream_attribute_ptr:
  case stream_attribute_string:
  case stream_attribute_blob:
    elementSize = sizeof(void*);
  default:
    return MA_TYPE_MISMATCH;
  }

  unsigned int bufLen = elementSize * elementCount;

  stream_attribute* pAtt = FindAttribute(id);
  if (pAtt) // Already exists. 
  {
    int attType = pAtt->type ^ stream_attribute_array;
    if (attType != elementType || !(pAtt->type & stream_attribute_array))
      return MA_TYPE_MISMATCH;

    if (pAtt->dataLen != bufLen) // If sizes don't match, need to release data from previous value.
    {
      free(pAtt->ptrVal);
      pAtt->ptrVal = malloc(bufLen);
      pAtt->dataLen = bufLen;
    }
    memcpy(pAtt->ptrVal, pElements, bufLen);
  }
  else
  {
    stream_attribute att;
    att.type = (stream_attribute_type)(elementType | stream_attribute_array);
    att.ptrVal = malloc(bufLen);
    att.dataLen = bufLen;
    memcpy(att.ptrVal, pElements, bufLen);
    m_Attributes[id] = att;
  }
  return MA_SUCCESS;
}

void CStreamAttributeCollection::Remove(MA_ATTRIB_ID id)
{
  // TODO: Implement
}

stream_attribute* CStreamAttributeCollection::FindAttribute(MA_ATTRIB_ID id)
{
  StreamAttributeIterator iter = m_Attributes.find(id);
  if (iter != m_Attributes.end())
    return &iter->second;
  return NULL;
}

// Test class to generate a sin waveform
////////////////////////////////////////////////////////////////////
const double CWaveGenerator::pi = 3.141592653589793238462643383279502884197169399375;
CWaveGenerator::CWaveGenerator(float freq) :
  m_Freq(freq),
  m_FramesRendered(0),
  m_Channels(0),
  m_SampleRate(0)
{

}

MA_RESULT CWaveGenerator::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if(!pDesc)
    return MA_ERROR;
  
  if (bus > 0)
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  int streamFormat = 0;
  int sampleType = 0;
  unsigned int bitDepth = 0;
  bool interleaved = false;
  if ((pAttribs->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, NULL) != MA_SUCCESS) ||
      (pAttribs->GetUInt(MA_ATT_TYPE_SAMPLERATE, NULL) != MA_SUCCESS) ||
      (pAttribs->GetInt(MA_ATT_TYPE_SAMPLE_TYPE, &sampleType) != MA_SUCCESS) ||
      (pAttribs->GetFlag(MA_ATT_TYPE_LPCM_FLAGS, MA_LPCM_FLAG_INTERLEAVED, &interleaved) != MA_SUCCESS) ||
      (pAttribs->GetUInt(MA_ATT_TYPE_BITDEPTH, &bitDepth) != MA_SUCCESS) ||
      (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_SEC, &streamFormat) != MA_SUCCESS))
    return MA_MISSING_ATTRIBUTE;

  // For now, we only support 16-bit, interleaved
  if ((streamFormat != MA_STREAM_FORMAT_LPCM) ||
      (sampleType != MA_SAMPLE_TYPE_SINT) ||
      (bitDepth != 16) ||
      (!interleaved))
    return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CWaveGenerator::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if(!pDesc)
    return MA_ERROR;
  
  if (bus > 0)
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  if ((pAttribs->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &m_Channels) != MA_SUCCESS) ||
      (pAttribs->GetUInt(MA_ATT_TYPE_SAMPLERATE, &m_SampleRate) != MA_SUCCESS))
    return MA_MISSING_ATTRIBUTE;

  return MA_SUCCESS;
}

MA_RESULT CWaveGenerator::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  if (!pOutput || pOutput->buffer_count == 0)
    return MA_ERROR;

  unsigned int bytesToRender = frameCount * 2 * m_Channels;
  if (pOutput->buffer[0].allocated_len < (frameCount * m_Channels * 2))
    return MA_ERROR;

  double amplitude = 0.7;
  short* pFrame = (short*)pOutput->buffer[0].data;
  for (unsigned int i = m_FramesRendered; i < m_FramesRendered + frameCount; i++)
  {
    short sample = (short)(amplitude * sin(((2 * pi * (double)m_Freq) / (double)m_SampleRate) * i) * 32767);
    for (unsigned int c = 0; c < m_Channels; c++)
    {
      *pFrame = sample;
      pFrame++;
    }
  }
  m_FramesRendered += frameCount;
  pOutput->buffer[0].data_len = bytesToRender;

  return MA_SUCCESS;
}
