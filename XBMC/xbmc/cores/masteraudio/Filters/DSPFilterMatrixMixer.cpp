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

/*
  Supports non-interleaved, 16-bit LPCM input and output <= 6 channels
*/

#include "stdafx.h"
#include "DSPFilterMatrixMixer.h"
#include <vector>

// Default channel layouts
// TODO: Consider making this platform-specific
int ma_layout_1ch[] = {MA_CHANNEL_FRONT_CENTER}; // Mono
int ma_layout_2ch[] = {MA_CHANNEL_FRONT_LEFT,    // Stereo
                       MA_CHANNEL_FRONT_RIGHT};
int ma_layout_3ch[] = {MA_CHANNEL_FRONT_LEFT,    // 2.1
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_LFE};
int ma_layout_4ch[] = {MA_CHANNEL_FRONT_LEFT,    // 4.0
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_REAR_LEFT,
                       MA_CHANNEL_REAR_RIGHT};
int ma_layout_5ch[] = {MA_CHANNEL_FRONT_LEFT,    // 5.0
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_FRONT_CENTER,
                       MA_CHANNEL_REAR_LEFT,
                       MA_CHANNEL_REAR_RIGHT};
int ma_layout_6ch[] = {MA_CHANNEL_FRONT_LEFT,    // 5.1
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_FRONT_CENTER,
                       MA_CHANNEL_LFE,
                       MA_CHANNEL_REAR_LEFT,
                       MA_CHANNEL_REAR_RIGHT};
int ma_layout_7ch[] = {MA_CHANNEL_FRONT_LEFT,    // 6.1
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_FRONT_CENTER,
                       MA_CHANNEL_LFE,
                       MA_CHANNEL_REAR_LEFT,
                       MA_CHANNEL_REAR_RIGHT,
                       MA_CHANNEL_REAR_CENTER};
int ma_layout_8ch[] = {MA_CHANNEL_FRONT_LEFT,    // 7.1
                       MA_CHANNEL_FRONT_RIGHT,
                       MA_CHANNEL_FRONT_CENTER,
                       MA_CHANNEL_LFE,
                       MA_CHANNEL_REAR_LEFT,
                       MA_CHANNEL_REAR_RIGHT,
                       MA_CHANNEL_FRONT_LOC,
                       MA_CHANNEL_FRONT_ROC};

int* ma_default_layout[] = {NULL,
                            ma_layout_1ch,
                            ma_layout_2ch,
                            ma_layout_3ch,
                            ma_layout_4ch,
                            ma_layout_5ch,
                            ma_layout_6ch,
                            ma_layout_7ch,
                            ma_layout_8ch};

CDSPFilterMatrixMixer::CDSPFilterMatrixMixer() :
  CDSPFilterLPCM(1,1),
  m_pInputLayout(NULL),
  m_pOutputLayout(NULL),
  m_pRouteList(NULL),
  m_RouteCount(0),
  m_pInputContainer(NULL)
{

}

CDSPFilterMatrixMixer::~CDSPFilterMatrixMixer()
{
  free(m_pInputLayout);
  free(m_pOutputLayout);
  delete[] m_pRouteList;
  ma_free_container(m_pInputContainer);
}

// TODO: Implement TestInputFormat

MA_RESULT CDSPFilterMatrixMixer::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT res;
  if ((res = CDSPFilterLPCM::SetInputFormat(pDesc, bus)) != MA_SUCCESS)
    return res;

  const LPCMAttributes* pAttribs = GetLPCMInputAttributes(bus);
  if (!pAttribs->m_IsInterleaved ||
     (pAttribs->m_SampleType != MA_SAMPLE_TYPE_SINT) ||
     (pAttribs->m_BitDepth != 16) ||
     (pAttribs->m_ChannelCount > 8))
     return MA_NOT_SUPPORTED;

  if (m_pInputLayout)
    free(m_pInputLayout);

  m_pInputLayout = GetChannelLayout(pDesc);
  // If the output format has already been set create or update the route list
  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    UpdateRouteList();

  return MA_SUCCESS;
}

// TODO: Implement TestOutputFormat

MA_RESULT CDSPFilterMatrixMixer::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT res;
  if ((res = CDSPFilterLPCM::SetOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return res;

  const LPCMAttributes* pAttribs = GetLPCMInputAttributes(bus);
  if (!pAttribs->m_IsInterleaved ||
     (pAttribs->m_SampleType != MA_SAMPLE_TYPE_SINT) ||
     (pAttribs->m_BitDepth != 16) ||
     (pAttribs->m_ChannelCount > MA_MAX_CHANNELS))
     return MA_NOT_SUPPORTED;

   if (m_pOutputLayout)
    free(m_pOutputLayout);

  m_pOutputLayout = GetChannelLayout(pDesc);
 
  // If the input format has already been set create or update the route list
  if (GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    UpdateRouteList();

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterMatrixMixer::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  if (bus >= m_OutputBusses)
    return MA_INVALID_BUS;

  unsigned int inChannels = GetLPCMInputAttributes(bus)->m_ChannelCount;
  unsigned int outChannels = GetLPCMOutputAttributes(bus)->m_ChannelCount;

  // Pass-through if no routes are defined and the input/output channels match
  // TODO: Can we limp though this if channel counts are different?
  if (!m_RouteCount)
  {
    if (inChannels == outChannels)
      return CDSPFilter::Render(pOutput, frameCount, renderTime, renderFlags, bus);
    return MA_ERROR;
  }

  unsigned int bytesToRender = GetInputAttributes(bus)->m_FrameSize * frameCount;
  // Validate input container size
  if (!m_pInputContainer)
  {
    m_pInputContainer = ma_alloc_container(1, GetInputAttributes(bus)->m_FrameSize, frameCount);
  }
  else if (m_pInputContainer->buffer[0].allocated_len < bytesToRender)
  {
    ma_free_container(m_pInputContainer);
    m_pInputContainer = ma_alloc_container(1, GetInputAttributes(bus)->m_FrameSize, frameCount);
  }

  MA_RESULT res;
  if ((res = GetInputData(m_pInputContainer, frameCount, renderTime, renderFlags, bus)) != MA_SUCCESS)
    return res;
  
  // Use floats as working type to provide headroom for mixing.
  short* pInputData = (short*)m_pInputContainer->buffer[0].data;
  short* pOutputData = (short*)pOutput->buffer[0].data;
  float mixArea[MA_MAX_CHANNELS];

  for (unsigned int frame = 0; frame < frameCount; frame++)
  {
    // TODO: Is there a more efficient way to silence channels with no routes? Maybe when creating the routes?
    memset(mixArea, 0, sizeof(float) * MA_MAX_CHANNELS);
    for (unsigned int route = 0; route < m_RouteCount; route++)
    {
      MixRoute* pRoute = &m_pRouteList[route];
      mixArea[pRoute->m_ToIndex] += pInputData[pRoute->m_FromIndex] * pRoute->m_Coeff; // Mix into work area
    }

    // TODO: Normalize matrix during creation
    // For now just clip
    for (unsigned int chan = 0; chan < outChannels; chan++)
    {
      if (mixArea[chan] > 32767.0f)
        pOutputData[chan] = 0;//(short)32767;
      else if (mixArea[chan] < -32768.0f)
        pOutputData[chan] = 0;//(short)-32768;
      else
        pOutputData[chan] = (short)mixArea[chan];
    }

    pInputData += inChannels;
    pOutputData += outChannels;
  }

  pOutput->buffer[0].data_len = GetOutputAttributes(bus)->m_FrameSize * frameCount;
  return MA_SUCCESS;
}

int* CDSPFilterMatrixMixer::GetChannelLayout(CStreamDescriptor* pDesc)
{
  CStreamAttributeCollection* pAttribs = pDesc->GetAttributes();
  unsigned int channelCount = 0;
  if (MA_SUCCESS != pAttribs->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &channelCount))
    return NULL;

  unsigned int bufSize = channelCount * sizeof(int);
  int* pLayout = (int*)malloc(bufSize);
  if (MA_SUCCESS != pAttribs->GetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, pLayout, bufSize))
    memcpy(pLayout, ma_default_layout[channelCount], bufSize); // Use defaults
  return pLayout;
}

#define ADD_ROUTE(list, route, in, out, coeff) \
        route.m_FromIndex = in; \
        route.m_ToIndex = out; \
        route.m_Coeff = coeff; \
        list.push_back(route);

// TODO: How can we let external callers override the heuristic mappings
// TODO: Implement matrix normalization to prevent overflows and clipping
void CDSPFilterMatrixMixer::UpdateRouteList()
{
  if (m_pRouteList) // If we already have routes, delete them
    delete[] m_pRouteList;
  m_pRouteList = NULL;

  if (!m_pInputLayout || !m_pOutputLayout) // Cannot build routes without layouts
    return;

  unsigned int inChannels = GetLPCMInputAttributes(0)->m_ChannelCount;
  unsigned int outChannels = GetLPCMOutputAttributes(0)->m_ChannelCount;
  if ((inChannels == outChannels) && // If channel layouts are identical, no need for mapping
     (!(memcmp(m_pInputLayout, m_pOutputLayout, outChannels * sizeof(int)))))
      return;

  // Build a location <-> layout cross reference
  // The channel map contains all possible channel locations and maps the 
  // pre-defined channel locations to array indexes in the layout
  int inMap[MA_MAX_CHANNELS];
  memset(inMap, MA_CHANNEL_NONE, sizeof(int) * MA_MAX_CHANNELS);
  int outMap[MA_MAX_CHANNELS];
  memset(outMap, MA_CHANNEL_NONE, sizeof(int) * MA_MAX_CHANNELS);

  for (unsigned int chan = 0; chan < inChannels; chan++)
    inMap[m_pInputLayout[chan]] = chan; // Channel 'x' is at position 'chan' in the layout

  for (unsigned int chan = 0; chan < outChannels; chan++)
    outMap[m_pOutputLayout[chan]] = chan; // Channel 'x' is at position 'chan' in the layout

  // Allocate the channel-mapping matrix
  // matrix[x][y] defines the mixing coefficient for input index 'x' into output index 'y'
  // The input/output maps are used to convert channel locations to input/output indexes
  // TODO: Allocate dynamically based on input/output channel counts
  float matrix[MA_MAX_CHANNELS][MA_MAX_CHANNELS];
  memset(matrix, 0, MA_MAX_CHANNELS * MA_MAX_CHANNELS * sizeof(float));
  #define SET_COEFF(inChan, outChan, coeff) matrix[inMap[inChan]][outMap[outChan]] = coeff

  int routeCount = 0;
  // TODO: Use a negative coefficient to invert phase when mixing Mono -> Stereo
  for (unsigned int chan = 0; chan < MA_MAX_CHANNELS; chan++)
  {
    //  1. Channels that exist in both layouts get mapped directly
    if (inMap[chan] != MA_CHANNEL_NONE && outMap[chan] != MA_CHANNEL_NONE)
    {
      SET_COEFF(chan, chan, 1.0f);
      routeCount++;
    }
    //  2. Channels that exist at input but not output must be downmixed
    // TODO: Perhaps this maxtrix should be in a predefined array
    // TODO: It may be best to validate the available channels against a set of sane options and drop channels when not sane
    // TODO: Coefficients should vary based on the number of channels
    else if (inMap[chan] != MA_CHANNEL_NONE && outMap[chan] == MA_CHANNEL_NONE)
    {
      switch(chan)
      {
      case MA_CHANNEL_FRONT_LEFT:
        if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(chan,MA_CHANNEL_FRONT_CENTER,0.5f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE)  {SET_COEFF(chan,MA_CHANNEL_FRONT_RIGHT,0.5f); routeCount++;}
       break;
      case MA_CHANNEL_FRONT_RIGHT:
        if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(chan,MA_CHANNEL_FRONT_CENTER,0.5f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE)   {SET_COEFF(chan,MA_CHANNEL_FRONT_LEFT,0.5f); routeCount++;}
       break;
      case MA_CHANNEL_REAR_LEFT:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE)   {SET_COEFF(chan,MA_CHANNEL_FRONT_LEFT,0.7f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(chan,MA_CHANNEL_FRONT_CENTER,0.5f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE)  {SET_COEFF(chan,MA_CHANNEL_FRONT_RIGHT,0.7f); routeCount++;}
        break;
      case MA_CHANNEL_REAR_RIGHT:
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE)  {SET_COEFF(chan,MA_CHANNEL_FRONT_RIGHT,0.7f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(chan,MA_CHANNEL_FRONT_CENTER,0.5f); routeCount++;}
        else if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE)   {SET_COEFF(chan,MA_CHANNEL_FRONT_LEFT,0.7f); routeCount++;}
        break;
      case MA_CHANNEL_FRONT_CENTER:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE)   {SET_COEFF(chan,MA_CHANNEL_FRONT_LEFT,0.5f); routeCount++;}
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE)  {SET_COEFF(chan,MA_CHANNEL_FRONT_RIGHT,0.5f); routeCount++;}
        break;
      case MA_CHANNEL_LFE: // TODO: Mix into all other channels? Just fronts?
      case MA_CHANNEL_SIDE_LEFT:
      case MA_CHANNEL_SIDE_RIGHT:
      case MA_CHANNEL_REAR_CENTER:
        break; // Drop these input channels
      }
    }
    //  3. Channels that exist at output but not input must be upmixed
    else if (inMap[chan] == MA_CHANNEL_NONE && outMap[chan] != MA_CHANNEL_NONE)
    {
      switch(chan)
      {
      case MA_CHANNEL_FRONT_LEFT:
        if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_CENTER,chan,1.0f); routeCount++;}
       break;
      case MA_CHANNEL_FRONT_RIGHT:
        if (outMap[MA_CHANNEL_FRONT_CENTER] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_CENTER,chan,1.0f); routeCount++;}
       break;
      case MA_CHANNEL_REAR_LEFT:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_LEFT,chan,0.5f); routeCount++;}
        break;
      case MA_CHANNEL_REAR_RIGHT:
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_RIGHT,chan,0.5f); routeCount++;}
        break;
      case MA_CHANNEL_FRONT_CENTER:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_LEFT,chan,0.5f); routeCount++;}
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_RIGHT,chan,0.5f); routeCount++;}
        break;
      case MA_CHANNEL_LFE: // TODO: Don't we need a Low-pass filter to properly handle this channel? Might be able to leave to the rest of the chain.
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_LEFT,chan,0.5f); routeCount++;}
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_RIGHT,chan,0.5f); routeCount++;}
        break;
      case MA_CHANNEL_SIDE_LEFT:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_LEFT,chan,0.25f); routeCount++;}
        if (outMap[MA_CHANNEL_REAR_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_REAR_LEFT,chan,0.25f); routeCount++;}
        break;
      case MA_CHANNEL_SIDE_RIGHT:
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_RIGHT,chan,0.25f); routeCount++;}
        if (outMap[MA_CHANNEL_REAR_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_REAR_RIGHT,chan,0.25f); routeCount++;}
        break;
      case MA_CHANNEL_REAR_CENTER:
        if (outMap[MA_CHANNEL_FRONT_LEFT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_LEFT,chan,0.25f); routeCount++;}
        if (outMap[MA_CHANNEL_FRONT_RIGHT] != MA_CHANNEL_NONE) {SET_COEFF(MA_CHANNEL_FRONT_RIGHT,chan,0.25f); routeCount++;}
        break;
      }
    }
  }

  // TODO: Adjust coefficients to maintain consistent input channel volume
  // In general, when we route one channel to N channels, to keep the loudness at the same level we must apply 1/sqrt(N) gain
  // TODO: Normalize coefficients to prevent clipping of output channels - Sum(coeff) <= 1.0f

  // TODO: Replace with simple two-dimensional array
  m_RouteCount = routeCount;
  m_pRouteList = new MixRoute[m_RouteCount];
  routeCount = 0;
  for (unsigned int inChan = 0; inChan < inChannels; inChan++)
  {
    for (unsigned int outChan = 0; outChan < outChannels; outChan++)
    {
      if (matrix[inChan][outChan] != 0.0f)
      {
        m_pRouteList[routeCount].m_FromIndex = inChan;
        m_pRouteList[routeCount].m_ToIndex = outChan;
        m_pRouteList[routeCount].m_Coeff = matrix[inChan][outChan];
        routeCount++;
      }
    }
  }
}
