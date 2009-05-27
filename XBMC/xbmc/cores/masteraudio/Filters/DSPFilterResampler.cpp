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
  m_InputDataFrames(0),
  m_pOutputData(NULL),
  m_OutputDataFrames(0),
  m_RemainingFrames(0)
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

  unsigned int channels = GetLPCMInputAttributes(0)->m_ChannelCount;

  // Create/Update output buffer
  if (m_OutputDataFrames < frameCount)
  {
    free(m_pOutputData);
    m_pOutputData = (float*)malloc(frameCount * channels * sizeof(float));
    m_OutputDataFrames = frameCount;
  }

  MA_RESULT res;
  unsigned int renderedFrames = 0;
  short* pRenderBuffer = (short*)pOutput->buffer[0].data;
  float* pOutputBuffer = m_pOutputData;

  // Determine if we already have a partially-complete render job
  if (m_RemainingFrames)
  {
    renderedFrames = m_RemainingFrames;
    src_float_to_short_array(pOutputBuffer, pRenderBuffer, renderedFrames * channels);
    pOutputBuffer += (renderedFrames * channels);
    pRenderBuffer += (renderedFrames * channels);
    m_RemainingFrames = 0;
  }

  while (renderedFrames < frameCount) // TODO: This is scary. Find a better way to handle repeat calls
  {
    int outputFrames = frameCount - renderedFrames; // Remaining frames to render. This should either be 0 or > 1, since GetFrames handles off-by-one.
    if ((res = GetFrames(&outputFrames, pOutputBuffer, channels, renderTime, renderFlags, bus)) != MA_SUCCESS)
    {
      m_RemainingFrames = renderedFrames; // Store unused frames for the next call
      return res;
    }
    renderedFrames += outputFrames; // Update render counter
    src_float_to_short_array(pOutputBuffer, pRenderBuffer, outputFrames * channels); // Convert float output data into short render data
    pOutputBuffer += (outputFrames * channels); // Update float data pointer
    pRenderBuffer += (outputFrames * channels); // Update render pointer
  }
  m_RemainingFrames = 0;
  pOutput->buffer[0].data_len = renderedFrames * sizeof(short) * channels;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterResampler::GetFrames(int* pOutputFrames, float* pOutputData, unsigned int channels, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  // Calculate data requirements
  unsigned int outputFrames = (unsigned int)*pOutputFrames;
  unsigned int inputFrames = (unsigned int)(outputFrames / m_Ratio);

  // Create/Update input buffer
  if (m_InputDataFrames < inputFrames)
  {
    free(m_pInputData);
    m_pInputData = (float*)malloc(inputFrames * channels * sizeof(float));
    m_InputDataFrames = inputFrames;
  }

  // Create/Update input container
  if (!m_pInputContainer || m_pInputContainer->buffer[0].allocated_len < (sizeof(short) * channels * inputFrames))
  {
    ma_free_container(m_pInputContainer);
    m_pInputContainer = ma_alloc_container(1, sizeof(short) * channels, inputFrames);
  }
  
  // Pull input data from the connected source
  MA_RESULT res;
  if ((res = GetInputData(m_pInputContainer, inputFrames, renderTime, renderFlags, bus)) != MA_SUCCESS)
  {
    *pOutputFrames = 0;
    return res;
  }

  // Convert input data to floats for libsamplerate
  src_short_to_float_array((short*)m_pInputContainer->buffer[0].data, m_pInputData, inputFrames * channels);

  // Prepare sample rate converter request
  SRC_DATA data;
  data.data_in = m_pInputData;
  data.data_out = pOutputData;
  data.input_frames = inputFrames;
  data.output_frames = outputFrames;
  data.src_ratio = m_Ratio;
  data.end_of_input = 0;
  int err = src_process(m_Converter, &data);
  if (err)
  {
    CLog::Log(LOGERROR, "%s: Error in libsamplerate!src_process: %s", __FUNCTION__, src_strerror(err));
    return MA_ERROR; // TODO: We shouldn't just dump the input data, should we?
  }
  if ((outputFrames - data.output_frames_gen) == 1) // Handle off-by one results internally by copying the last frame
  {
    memcpy(data.data_out + (data.output_frames_gen * channels), data.data_out + ((data.output_frames_gen - 1)* channels), sizeof(float) * channels);
    data.output_frames_gen++;
  }
  *pOutputFrames = data.output_frames_gen;
  return MA_SUCCESS;
}

void CDSPFilterResampler::CleanUp()
{
  if (m_Converter)
    m_Converter = src_delete(m_Converter);
  ma_free_container(m_pInputContainer);
  m_pInputContainer = NULL;
  free(m_pInputData);
  m_pInputData = NULL;
  free(m_pOutputData);
  m_pOutputData = NULL;
}
