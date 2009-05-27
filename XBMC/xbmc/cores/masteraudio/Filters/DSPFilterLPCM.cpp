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
#include "DSPFilterLPCM.h"

CDSPFilterLPCM::CDSPFilterLPCM(unsigned int inputBusses /* = 1*/, unsigned int outputBusses /* = 1*/) :
  CDSPFilter(inputBusses, outputBusses)
{
  m_pLPCMInputDescriptor = (LPCMAttributes*)calloc(inputBusses , sizeof(LPCMAttributes));
  m_pLPCMOutputDescriptor = (LPCMAttributes*)calloc(outputBusses, sizeof(LPCMAttributes));
}

CDSPFilterLPCM::~CDSPFilterLPCM()
{
  free(m_pLPCMInputDescriptor);
  free(m_pLPCMOutputDescriptor);
}

void CDSPFilterLPCM::ClearInputFormat(unsigned int bus /* = 0 */)
{
  CDSPFilter::ClearInputFormat(bus);
  if (bus < m_InputBusses)
    memset(&m_pLPCMInputDescriptor[bus], 0, sizeof(LPCMAttributes));
}

void CDSPFilterLPCM::ClearOutputFormat(unsigned int bus /* = 0 */)
{
  CDSPFilter::ClearOutputFormat(bus);
  if (bus < m_OutputBusses)
    memset(&m_pLPCMOutputDescriptor[bus], 0, sizeof(LPCMAttributes));
}

MA_RESULT CDSPFilterLPCM::ReadLPCMAttributes(CStreamDescriptor *pDesc, LPCMAttributes *pLPCM)
{
  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  if (
    (pAttribs->GetFlag (MA_ATT_TYPE_LPCM_FLAGS    , MA_LPCM_FLAG_INTERLEAVED, &pLPCM->m_IsInterleaved) != MA_SUCCESS) ||
    (pAttribs->GetInt  (MA_ATT_TYPE_SAMPLE_TYPE   , &pLPCM->m_SampleType   ) != MA_SUCCESS) ||
    (pAttribs->GetUInt (MA_ATT_TYPE_BITDEPTH      , &pLPCM->m_BitDepth     ) != MA_SUCCESS) ||
    (pAttribs->GetUInt (MA_ATT_TYPE_SAMPLERATE    , &pLPCM->m_SampleRate   ) != MA_SUCCESS) ||
    (pAttribs->GetUInt (MA_ATT_TYPE_CHANNEL_COUNT , &pLPCM->m_ChannelCount ) != MA_SUCCESS) ||
    (pAttribs->GetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, &pLPCM->m_ChannelLayout, sizeof(pLPCM->m_ChannelLayout)) != MA_SUCCESS))
    return MA_MISSING_ATTRIBUTE;

  return MA_SUCCESS;
}

// IAudioSink
MA_RESULT CDSPFilterLPCM::TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilter::TestInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  int streamFormat = MA_STREAM_FORMAT_UNKNOWN;
  if ((pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT, &streamFormat) != MA_SUCCESS) || 
      (streamFormat != MA_STREAM_FORMAT_LPCM))
    return MA_NOT_SUPPORTED;

  // Verify that the required attributes are present and of the correct type
  LPCMAttributes LPCM;
  return ReadLPCMAttributes(pDesc, &LPCM);
}

MA_RESULT CDSPFilterLPCM::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilter::SetInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  // Fetch and store the required stream attributes
  if ((ret = ReadLPCMAttributes(pDesc, &m_pLPCMInputDescriptor[bus])) != MA_SUCCESS)
  {
    ClearInputFormat(bus);
    return ret;
  }
 
  return MA_SUCCESS;
}

// IAudioSource
MA_RESULT CDSPFilterLPCM::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilter::TestOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  int streamFormat = MA_STREAM_FORMAT_UNKNOWN;
  if ((pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT, &streamFormat) != MA_SUCCESS) || 
      (streamFormat != MA_STREAM_FORMAT_LPCM))
    return MA_NOT_SUPPORTED;

  LPCMAttributes LPCM;
  return ReadLPCMAttributes(pDesc, &LPCM);
}

MA_RESULT CDSPFilterLPCM::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilter::SetOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  // Fetch and store the required stream attributes
  if ((ret = ReadLPCMAttributes(pDesc, &m_pLPCMOutputDescriptor[bus])) != MA_SUCCESS)
  {
    ClearOutputFormat(bus);
    return ret;
  }

  return MA_SUCCESS;
}