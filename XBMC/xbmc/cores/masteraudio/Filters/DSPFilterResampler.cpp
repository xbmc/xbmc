/*
 *      Copyright (C) 2009 Team XBMC
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
#include "DSPFilterResampler.h"

CDSPFilterResampler::CDSPFilterResampler() :
  CDSPFilterLPCM(1,1),
  m_pInputContainer(NULL),
  m_Converter(NULL),
  m_Ratio(1.0),
  m_pInputData(NULL),
  m_pOutputData(NULL)
{

}

CDSPFilterResampler::~CDSPFilterResampler()
{
  CleanUp();
}

// TODO: Implement TestInputFormat

MA_RESULT CDSPFilterResampler::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT res;
  if ((res = CDSPFilterLPCM::SetInputFormat(pDesc, bus)) != MA_SUCCESS)
    return res;

  const LPCMAttributes* pAttribs = GetLPCMInputAttributes(bus);
  if (!pAttribs->m_IsInterleaved ||
     (pAttribs->m_SampleType != MA_SAMPLE_TYPE_SINT) ||
     (pAttribs->m_BitDepth != 16))
     return MA_NOT_SUPPORTED;

  // If the output format has already been set create or update the route list
  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
  {
    if ((res = Init()) != MA_SUCCESS)
      return res;
  }

  return MA_SUCCESS;
}

// TODO: Implement TestOutputFormat

MA_RESULT CDSPFilterResampler::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT res;
  if ((res = CDSPFilterLPCM::SetOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return res;

  const LPCMAttributes* pAttribs = GetLPCMInputAttributes(bus);
  if (!pAttribs->m_IsInterleaved ||
     (pAttribs->m_SampleType != MA_SAMPLE_TYPE_SINT) ||
     (pAttribs->m_BitDepth != 16))
     return MA_NOT_SUPPORTED;

  // If the input format has already been set create or update the route list
  if (GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
  {
    if ((res = Init()) != MA_SUCCESS)
      return res;
  }
  return MA_SUCCESS;
}

MA_RESULT CDSPFilterResampler::Init()
{
  CleanUp();

  unsigned int inChannels = GetLPCMInputAttributes(0)->m_ChannelCount;
  unsigned int outChannels = GetLPCMInputAttributes(0)->m_ChannelCount;
  if (inChannels != outChannels)
    return MA_NOT_SUPPORTED;

  // TODO: How do we decide which converter to use?
  int err = 0;
  m_Converter = src_new(SRC_SINC_MEDIUM_QUALITY, inChannels, &err);
  if (!m_Converter)
    return MA_ERROR;

  m_Ratio = (double)GetLPCMOutputAttributes(0)->m_SampleRate / (double)GetLPCMInputAttributes(0)->m_SampleRate;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterResampler::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  if (bus >= m_OutputBusses)
    return MA_INVALID_BUS;

  int inputFrames = (int)((double)frameCount / m_Ratio);
  unsigned int channels = GetLPCMInputAttributes(0)->m_ChannelCount;
  unsigned int inputBytes = GetInputAttributes(bus)->m_FrameSize * inputFrames;
  // Validate input container size
  if (!m_pInputContainer)
  {
    m_pInputContainer = ma_alloc_container(1, GetInputAttributes(bus)->m_FrameSize, inputFrames);
    m_pInputData = (float*)malloc(inputFrames * channels * sizeof(float));
    m_pOutputData = (float*)malloc(frameCount * channels * sizeof(float));
  }
  else if (m_pInputContainer->buffer[0].allocated_len < inputBytes)
  {
    ma_free_container(m_pInputContainer);
    m_pInputContainer = ma_alloc_container(1, GetInputAttributes(bus)->m_FrameSize, inputFrames);
    free(m_pInputData);
    free(m_pOutputData);
    m_pInputData = (float*)malloc(inputFrames * channels * sizeof(float));
    m_pOutputData = (float*)malloc(frameCount * channels * sizeof(float));
  }
  
  MA_RESULT res;
  if ((res = GetInputData(m_pInputContainer, inputFrames, renderTime, renderFlags, bus)) != MA_SUCCESS)
    return res;

  // Push input data into float buffer
  src_short_to_float_array((short*)m_pInputContainer->buffer[0].data, m_pInputData, inputFrames * channels);

  // Convert frames
  SRC_DATA data;
  data.data_in = m_pInputData;
  data.data_out = m_pOutputData;
  data.input_frames = inputFrames;
  data.output_frames = frameCount;
  data.src_ratio = m_Ratio;
  data.end_of_input = 0;
  int err = src_process(m_Converter, &data);
  if (err)
  {
    CLog::Log(LOGERROR, "%s: Error in libsamplerate!src_process: %s", __FUNCTION__, src_strerror(err));
    return MA_ERROR; // TODO: We shouldn't just dump the input data
  }
  if ((frameCount - data.output_frames_gen) > 1 )
  {
    int deltaFramesOut = frameCount - data.output_frames_gen;
    int deltaFramesIn = (int)((double)deltaFramesOut / m_Ratio);
    ma_audio_container* pCont = ma_alloc_container(1, GetInputAttributes(bus)->m_FrameSize, deltaFramesIn);
    if ((res = GetInputData(pCont, deltaFramesIn, renderTime, renderFlags, bus)) != MA_SUCCESS)
    {
      ma_free_container(pCont);
      return res;
    }
    src_short_to_float_array((short*)pCont->buffer[0].data, m_pInputData, deltaFramesIn * channels);
    ma_free_container(pCont); // Done with it now
    data.data_out = m_pOutputData + (data.output_frames_gen * channels);
    data.input_frames = deltaFramesIn;
    data.output_frames = deltaFramesOut;
    err = src_process(m_Converter, &data);
    if (err)
    {
      CLog::Log(LOGERROR, "%s: Error in libsamplerate!src_process: %s", __FUNCTION__, src_strerror(err));
      return MA_ERROR; // TODO: We shouldn't just dump the input data
    }
    if (deltaFramesOut - data.output_frames_gen)
      memcpy(data.data_out + (data.output_frames_gen * channels), data.data_out + ((data.output_frames_gen - 1)* channels), sizeof(float) * channels);
  }
  else
  {
    memcpy(m_pOutputData + (data.output_frames_gen * channels), m_pOutputData + ((data.output_frames_gen - 1)* channels), sizeof(float) * channels);
  }
  // Copy output data out of float buffer
  src_float_to_short_array(m_pOutputData, (short*)pOutput->buffer[0].data, frameCount * channels);
  pOutput->buffer[0].data_len = frameCount * GetOutputAttributes(bus)->m_FrameSize;

  return MA_SUCCESS;
}

void CDSPFilterResampler::CleanUp()
{
  if (m_Converter)
    m_Converter = src_delete(m_Converter);
  ma_free_container(m_pInputContainer);
  m_pInputContainer = NULL;
}
