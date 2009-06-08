/*
 *      Copyright (C) 2009 Team XBMC
 *
 *  This Program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DSPFilterAC3Encoder.h"

// This is the channel layout that ac3enc expects
int ac3layout[6][6] = {
  // 1 channel (Mono)
  {
    MA_CHANNEL_FRONT_CENTER
  },
  // 2 channels (2.0)
  {
    MA_CHANNEL_FRONT_LEFT,
    MA_CHANNEL_FRONT_RIGHT
  },
  // 3 channels (3.0)
  {
    MA_CHANNEL_FRONT_LEFT,
    MA_CHANNEL_FRONT_CENTER,
    MA_CHANNEL_FRONT_RIGHT
  },
  // 4 channels (4.0)
  {
    MA_CHANNEL_FRONT_LEFT,
    MA_CHANNEL_FRONT_RIGHT,
    MA_CHANNEL_REAR_LEFT,
    MA_CHANNEL_REAR_RIGHT
  },
  // 5 channels (5.0)
  {
    MA_CHANNEL_FRONT_LEFT,
    MA_CHANNEL_FRONT_CENTER,
    MA_CHANNEL_FRONT_RIGHT,
    MA_CHANNEL_REAR_LEFT,
    MA_CHANNEL_REAR_RIGHT
  },
  // 6 channels (5.1)
  {
    MA_CHANNEL_FRONT_LEFT,
    MA_CHANNEL_FRONT_CENTER,
    MA_CHANNEL_FRONT_RIGHT,
    MA_CHANNEL_REAR_LEFT,
    MA_CHANNEL_REAR_RIGHT,
    MA_CHANNEL_LFE
  }
};

CDSPFilterAC3Encoder::CDSPFilterAC3Encoder(unsigned int inputBusses /* = 1*/, unsigned int outputBusses /* = 1*/) :
  CDSPFilterLPCM(inputBusses, outputBusses)
{
  // Allocate our input container array
  m_pEncoderAttributes = (EncoderAttributes*)calloc(inputBusses, sizeof(EncoderAttributes));
}

CDSPFilterAC3Encoder::~CDSPFilterAC3Encoder()
{
  // Free all our input containers and clean up any remaining encoder data
  for(unsigned int bus = 0; bus < m_InputBusses; bus++) 
  {
    ma_free_container(m_pEncoderAttributes[bus].m_pInputContainer);
    if (m_pEncoderAttributes[bus].m_CodecContext != NULL)
    {
      m_AvCodec.avcodec_close(m_pEncoderAttributes[bus].m_CodecContext);
      m_AvUtil.av_free(m_pEncoderAttributes[bus].m_CodecContext);
    }
  }
  free(m_pEncoderAttributes);
}

// IAudioSink
MA_RESULT CDSPFilterAC3Encoder::TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  LPCMAttributes inputAtt;

  if ((ret = CDSPFilterLPCM::TestInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  if ((ret = ReadLPCMAttributes(pDesc, &inputAtt)) != MA_SUCCESS)
    return ret;

  if (
    (!inputAtt.m_IsInterleaved) ||
    (inputAtt.m_SampleType != MA_SAMPLE_TYPE_SINT) ||
    (inputAtt.m_BitDepth != 16) ||
    (inputAtt.m_ChannelCount > 6))
    return MA_NOT_SUPPORTED;

  // If the output format is set, the input sample rate must match the output sample rate
  if ((GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN) && 
    (inputAtt.m_SampleRate != m_pEncoderAttributes[bus].m_SampleRate))
    return MA_NOT_SUPPORTED;

  // Ensure the channel layout is compatible
  // TODO: patch ac3enc to allow us to set the acmod so we can support 2.1... etc
  for(unsigned int i = 0; i < inputAtt.m_ChannelCount; i++)
    if (ac3layout[inputAtt.m_ChannelCount - 1][i] != inputAtt.m_ChannelLayout[i])
      return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterAC3Encoder::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilterLPCM::SetInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  // If the output format has been set, intitialize the encoder
  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    return InitCodec(pDesc, bus);

  return MA_SUCCESS;
}

// IAudioSource
MA_RESULT CDSPFilterAC3Encoder::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT                  ret;
  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  unsigned int               outSampleRate;
  int                        encoding;

  if ((ret = CDSPFilter::TestOutputFormat(pDesc, bus)) != MA_SUCCESS) // We do not call CDSPFilterLPCM, our output is not LPCM
    return ret;

  // Our output format is IEC61937
  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_IEC61937)
    return MA_NOT_SUPPORTED;

  if (
    (pAttribs->GetInt(MA_ATT_TYPE_ENCODING, &encoding) != MA_SUCCESS) ||
    (pAttribs->GetUInt(MA_ATT_TYPE_SAMPLERATE, &outSampleRate) != MA_SUCCESS))
    return MA_MISSING_ATTRIBUTE;

  if (encoding != MA_STREAM_ENCODING_AC3)
    return MA_NOT_SUPPORTED;

  // If the input format has already been set, check for a matching sample rate
  if ((GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN) && 
    (outSampleRate != GetLPCMInputAttributes(bus)->m_SampleRate))
    return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterAC3Encoder::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilter::SetOutputFormat(pDesc, bus)) != MA_SUCCESS) // We do not call CDSPFilterLPCM, our output is not LPCM
    return ret;

  if ((ret = pDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &m_pEncoderAttributes[bus].m_SampleRate)) != MA_SUCCESS)
    return ret;

  // If the input format has been set, intitialize the encoder
  if (GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    return InitCodec(pDesc, bus);

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterAC3Encoder::InitCodec(CStreamDescriptor *pDesc, unsigned int bus)
{
  AVCodec* pCodec;
  EncoderAttributes* pEncoderAtt = &m_pEncoderAttributes[bus];

  // Init ffmpeg if necessary
  if (!m_AvUtil.IsLoaded())
    if (!m_AvUtil.Load())
      return MA_ERROR;
  if (!m_AvCodec.IsLoaded())
    if (!m_AvCodec.Load())
      return MA_ERROR;

  if (GetLPCMInputAttributes(bus)->m_SampleRate != pEncoderAtt->m_SampleRate)
    return MA_NOT_SUPPORTED;

  // Close any existing codec context
  if (pEncoderAtt->m_CodecContext)
  {
    m_AvCodec.avcodec_close(pEncoderAtt->m_CodecContext);
    m_AvUtil.av_free(pEncoderAtt->m_CodecContext);
    pEncoderAtt->m_CodecContext = NULL;
  }
 
  pCodec = m_AvCodec.avcodec_find_encoder(CODEC_ID_AC3);
  if (!pCodec)
  {
    CLog::Log(LOGERROR, "%s - Unable to find the AC3 encoder", __FUNCTION__);
    return MA_ERROR;
  }

  // Set-up the context used when interacting with the encoder
  pEncoderAtt->m_CodecContext              = m_AvCodec.avcodec_alloc_context();           // Allocate the context
  pEncoderAtt->m_CodecContext->bit_rate    = 448000;
  pEncoderAtt->m_CodecContext->sample_rate = pEncoderAtt->m_SampleRate;
  pEncoderAtt->m_CodecContext->channels    = GetLPCMInputAttributes(bus)->m_ChannelCount; // Channel Count
  pEncoderAtt->m_CodecContext->sample_fmt  = SAMPLE_FMT_S16;                              // Currently support only 16-bit Signed Integer

  // Try to open the codec
  if(m_AvCodec.avcodec_open(pEncoderAtt->m_CodecContext, pCodec) < 0)
  {
    m_AvUtil.av_free(pEncoderAtt->m_CodecContext);
    pEncoderAtt->m_CodecContext = NULL;
    CLog::Log(LOGERROR, "%s - Unable to open the AC3 encoder", __FUNCTION__);
    return MA_ERROR;
  }
  // TODO: Do we need to free pCodec?
  return MA_SUCCESS;
}

MA_RESULT CDSPFilterAC3Encoder::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  EncoderAttributes* pEncoderAtt = &m_pEncoderAttributes[bus];
  if (!pEncoderAtt->m_CodecContext)
    return MA_ERROR;

  // Ensure the output buffer is large enough
  if (pOutput->buffer[0].allocated_len < frameCount * AC3_FRAME_SIZE)
    return MA_ERROR;

  // Ensure our input buffer is large enough
  unsigned int inputFrames = AC3_SAMPLES_PER_FRAME * frameCount;
  unsigned int inputFrameSize = GetInputAttributes(bus)->m_FrameSize;
  if (!pEncoderAtt->m_pInputContainer || pEncoderAtt->m_pInputContainer->buffer[0].allocated_len < inputFrames * inputFrameSize)
  {
    if (pEncoderAtt->m_pInputContainer)
      ma_free_container(pEncoderAtt->m_pInputContainer);
    pEncoderAtt->m_pInputContainer = ma_alloc_container(1, inputFrameSize, inputFrames);
  }

  // Get the input frames from the upstream renderer
  MA_RESULT ret;
  ma_audio_container* pInput = pEncoderAtt->m_pInputContainer;
  if ((ret = GetInputData(pInput, inputFrames, renderTime, renderFlags, bus)) != MA_SUCCESS)
    return ret;

  // encode the data
  uint8_t* pOutFrame = (uint8_t*)pOutput->buffer[0].data;
  short* pInFrame = (short*)pInput ->buffer[0].data;
  unsigned int channelCount = GetLPCMInputAttributes(bus)->m_ChannelCount;
  pOutput->buffer[0].data_len = 0;
  for(unsigned int frame = 0; frame < frameCount; ++frame)
  {
    unsigned int encodedBytes = m_AvCodec.avcodec_encode_audio(pEncoderAtt->m_CodecContext, pOutFrame + 8, AC3_FRAME_SIZE * 4 - 8, pInFrame);

    // TODO: Do we need to output aligned frames?

    // setup the iec958 header (this is in little endian)
    pOutFrame[0] = 0xF8;
    pOutFrame[1] = 0x72;
    pOutFrame[2] = 0x4E;
    pOutFrame[3] = 0x1F;

    pOutFrame[4] = pOutFrame[13] & 7;
    pOutFrame[5] = (encodedBytes > 0) ? 0x01 : 0x00;
    pOutFrame[6] = ((encodedBytes * 8) >> 8) & 0xFF;
    pOutFrame[7] = (encodedBytes * 8) & 0xFF;
    encodedBytes += 8;

    // Zero the rest of the frame
    memset(pOutFrame + encodedBytes, 0, AC3_FRAME_SIZE * 4 - encodedBytes);

#ifdef WORDS_BIGENDIAN
    // Byteswap just the header to big endian
    swab((char*)pOutFrame, (char*)pOutFrame, 8);
#else
    // Byteswap the entire packet to big endian
    swab((char*)pOutFrame, (char*)pOutFrame, encodedBytes);
#endif

    // Advance to the next set of frames
    pInFrame += AC3_SAMPLES_PER_FRAME * channelCount;
    pOutFrame += AC3_FRAME_SIZE * 4;
    pOutput->buffer[0].data_len += AC3_FRAME_SIZE * 4;
  }

  return MA_SUCCESS;
}
