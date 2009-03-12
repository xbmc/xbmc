/*
 *      Copyright (C) 2009 phi2039
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
 */

#include "stdafx.h"
#include "AC3Encoder.h"

// TODO: This needs a better home
CCriticalSection LibAvCodec::m_critSection;

CAC3Encoder::CAC3Encoder() :
  m_pCodecContext(NULL),
  m_CodecIsOpen(false),
  m_pInputSlice(NULL),
  m_InputBufferPos(0),
  m_Channels(0)
{

}

CAC3Encoder::~CAC3Encoder()
{
  Close();
}

// IDSPFilter Methods
//////////////////////////////////////////
void CAC3Encoder::Close()
{
  CloseCodec();
}

// IAudioSink Methods
//////////////////////////////////////////
MA_RESULT CAC3Encoder::TestInputFormat(CStreamDescriptor* pDesc)
{
  if (!pDesc)
    return MA_ERROR;

  int format = 0;
  int sampleRate = 0;
  int bitsPerSample = 0;
  int channels = 0;

  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_STREAM_FORMAT,&format);
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_SAMPLESPERSEC,&sampleRate);
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_BITDEPTH,&bitsPerSample);
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_CHANNELS,&channels);
  
  if(format == MA_STREAM_FORMAT_PCM && sampleRate == 48000 && bitsPerSample == 16 && channels > 0 && channels < 7)
    return MA_SUCCESS;

  return MA_NOT_SUPPORTED;
}

MA_RESULT CAC3Encoder::SetInputFormat(CStreamDescriptor* pDesc)
{
  // Make sure we support the provided format
  if (MA_SUCCESS == TestInputFormat(pDesc))
  {
    // Store the number of channels in the stream and open the codec
    if (MA_SUCCESS == pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_CHANNELS, &m_Channels) && OpenCodec(m_Channels,48000,16))
    {
      m_FrameDataLen = m_pCodecContext->frame_size * m_pCodecContext->channels * 2 /*bytes per sample*/;
      return MA_SUCCESS;
    }
  }
  return MA_ERROR;
}

MA_RESULT CAC3Encoder::GetInputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps || ! m_pCodecContext) // Codec must be open
    return MA_ERROR;

  size_t inputSize = m_pCodecContext->frame_size * m_pCodecContext->channels * 2 /*bytes per sample*/;
  pProps->transfer_alignment = m_FrameDataLen; // Frame Data Size
  pProps->preferred_transfer_size = m_FrameDataLen; // One Frame
  pProps->max_transfer_size = 0; // No Maximum, as long as it is frame-aligned

  return MA_SUCCESS;
}

MA_RESULT CAC3Encoder::AddSlice(audio_slice* pSlice)
{
  // TODO: Reject data when output buffer is full
  if (!pSlice || !m_pCodecContext)
    return MA_ERROR;

  if (m_pInputSlice)
    return MA_BUSYORFULL; // We can only handle one at a time

  if (pSlice->header.data_len % m_FrameDataLen)
    return MA_NOT_SUPPORTED; // Input slices must be properly aligned or we will lose data

  m_pInputSlice = pSlice;

  return MA_SUCCESS;
}

float CAC3Encoder::GetMaxLatency()
{
  // TODO: Implement
  return 0.0f;
}

void CAC3Encoder::Flush()
{
  delete m_pInputSlice;
  m_pInputSlice = NULL;
}

// IAudioSource Methods
//////////////////////////////////////////
MA_RESULT CAC3Encoder::TestOutputFormat(CStreamDescriptor* pDesc)
{
  if (!pDesc)
    return MA_ERROR;

  // We only support one output format (we're an encoder)
  int format = 0;
  int encoding = 0;

  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_STREAM_FORMAT,&format);
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_ENCODING,&encoding);

  if (format == MA_STREAM_FORMAT_ENCODED && encoding == MA_STREAM_ENCODING_AC3)
    return MA_SUCCESS;

  return MA_NOT_SUPPORTED;
}

MA_RESULT CAC3Encoder::SetOutputFormat(CStreamDescriptor* pDesc)
{
  return TestOutputFormat(pDesc);
}

MA_RESULT CAC3Encoder::GetOutputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps || ! m_pCodecContext)
    return MA_ERROR;

  pProps->transfer_alignment = 0; // We don't know (variable)
  pProps->preferred_transfer_size = 0; // We don't know (variable)
  pProps->max_transfer_size = m_FrameDataLen; // No larger than 1 input frame

  return MA_SUCCESS;
}

MA_RESULT CAC3Encoder::GetSlice(audio_slice** ppSlice)
{
  if (!m_pInputSlice)
    return MA_NEED_DATA; // Nothing to encode

  // Make sure we have a complete input frame
  if (m_pInputSlice->header.data_len - m_InputBufferPos < m_FrameDataLen) // This should NEVER happen
  {
    // Drop the stragglers
    delete m_pInputSlice;
    m_pInputSlice = NULL;
    return MA_NEED_DATA;
  }

  // TODO: Use global slice allocator
  audio_slice* pOut = audio_slice::create_slice(m_FrameDataLen); // We don't know how much space we'll need, but never more than the input (I hope)

  // Encode an input frame
  size_t bytesWritten = m_pCodecContext->codec->encode(m_pCodecContext, pOut->get_data(), m_FrameDataLen, m_pInputSlice->get_data() + m_InputBufferPos);
  pOut->header.data_len = bytesWritten; // Update slice size to actual encoded length
  if (!bytesWritten)
  {
    delete pOut;
    return MA_ERROR;
  }
  m_InputBufferPos += m_FrameDataLen;
  if (m_InputBufferPos >= m_pInputSlice->header.data_len)
  {
    // We are done with this one
    delete m_pInputSlice;
    m_pInputSlice = NULL;
  }
  *ppSlice = pOut;

  return MA_SUCCESS;
}

// Codec Interface Functions
//////////////////////////////////////////
bool CAC3Encoder::OpenCodec(unsigned int inputChannels, unsigned int inputSamplesPerSec, unsigned int inputBitsPerSample)
{
  AVCodec* pCodec;
  if (!m_avLib.Load())
    return false;

  m_pCodecContext = m_avLib.avcodec_alloc_context();
  m_avLib.avcodec_get_context_defaults(m_pCodecContext);

  m_avLib.avcodec_register_all();
  pCodec = m_avLib.avcodec_find_encoder(CODEC_ID_AC3);
  if (!pCodec)
  {
    CloseCodec();
    return false;
  }

  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;
  m_pCodecContext->priv_data = new BYTE[pCodec->priv_data_size];

  if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

  m_pCodecContext->channels = inputChannels;
  m_pCodecContext->sample_rate = inputSamplesPerSec;
  m_pCodecContext->bit_rate = inputChannels * inputSamplesPerSec * (inputBitsPerSample >> 3);

  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  if (m_avLib.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CloseCodec();
    return false;
  }
  
  m_CodecIsOpen = true;
  return true;
}

void CAC3Encoder::CloseCodec()
{
  if (m_pCodecContext)
  {
    if (m_CodecIsOpen)
      m_avLib.avcodec_close(m_pCodecContext);
    // TODO: Free codec context
    // m_avLib.av_free(m_pCodecContext); // From libavutil
    delete[] m_pCodecContext->priv_data;
    m_pCodecContext = NULL;
  }
  m_avLib.Unload();
}