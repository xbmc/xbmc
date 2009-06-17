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
#include "DSPChain.h"
#include "Filters/DSPFilterMatrixMixer.h"
#include "Filters/DSPFilterResampler.h"
#include "Filters/DSPFilterAC3Encoder.h"
#include "Filters/TestFilter.h"

// CDSPChain
//////////////////////////////////////////////////////////////////////////////////////
CDSPChain::CDSPChain() :
  CDSPFilter(1,1),
  m_pHead(NULL),
  m_pTail(NULL)
{
Create(1,1);
}

CDSPChain::~CDSPChain()
{
  Close();
}

void CDSPChain::Create(unsigned int inputBusses, unsigned int outputBusses)
{

}

void CDSPChain::Destroy()
{

}

// TODO: Trigger graph creation on format change

MA_RESULT CDSPChain::CreateFilterGraph(CStreamDescriptor* pInDesc, CStreamDescriptor* pOutDesc)
{
  if (!pInDesc || !pOutDesc)
    return MA_ERROR;

  if (SetInputFormat(pInDesc) != MA_SUCCESS || 
      SetOutputFormat(pOutDesc) != MA_SUCCESS)
    return MA_NOT_SUPPORTED;

  CStdString filterChainText = "DSPChain: Input";

  m_pHead = m_pTail = NULL;

  // See if the stream wants to be passed-through untouched
  if (GetInputAttributes(0)->m_Locked)
  {
    CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Detected locked stream. No filters will be created or applied.");
  }
  else
  {
    CStreamDescriptor workingDesc = *pInDesc;

    // TODO: Dynamically create DSP Filter Graph 
    // TODO: Link elements properly and link at end
    bool installTest = false;

    // Install DSP Transform Test Container. Who knows what this does today?
    if (installTest)
    {
      dsp_filter_node* pPrev = m_pTail;
      m_pTail = new dsp_filter_node();
      m_pTail->filter = new CDSPFilterTest();
      m_pTail->filter->SetInputFormat(&workingDesc);
      m_pTail->filter->SetOutputFormat(&workingDesc); // This filter will not alter the format
      filterChainText += " -> TestFilter";
      if (pPrev)
        m_pTail->filter->SetSource(pPrev->filter);
      if (!m_pHead)
        m_pHead = m_pTail;
    }

    unsigned int outputFormat = pOutDesc->GetFormat();
    unsigned int inputFormat = workingDesc.GetFormat();

    // See if we need up/down-mixing or channel re-mapping
    unsigned int inChannels = 0;
    unsigned int outChannels = 0;
    workingDesc.GetAttributes()->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &inChannels);
    pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &outChannels);

    int inLayout[MA_MAX_CHANNELS];
    int outLayout[MA_MAX_CHANNELS];
    memset(inLayout, 0, sizeof(int) * MA_MAX_CHANNELS);
    memset(outLayout, 0, sizeof(int) * MA_MAX_CHANNELS);
    workingDesc.GetAttributes()->GetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, inLayout, sizeof(int) * MA_MAX_CHANNELS);
    pOutDesc->GetAttributes()->GetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, outLayout, sizeof(int) * MA_MAX_CHANNELS);

    if (((inChannels != outChannels || memcmp(inLayout, outLayout, sizeof(int) * MA_MAX_CHANNELS)) && outputFormat != MA_STREAM_FORMAT_IEC61937) ||
       (outputFormat == MA_STREAM_FORMAT_IEC61937 && inChannels < 6))
    {
      dsp_filter_node* pPrev = m_pTail;
      m_pTail = new dsp_filter_node();
      m_pTail->filter = new CDSPFilterMatrixMixer();
      m_pTail->filter->SetInputFormat(&workingDesc);

      // Update input format to reflect conversion performed by filter
      if (outputFormat == MA_STREAM_FORMAT_IEC61937)
      {
        unsigned int ac3Channels = 6;
        workingDesc.GetAttributes()->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT, ac3Channels); // Update to reflect conversion performed by filter
        workingDesc.GetAttributes()->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, ac3Channels * 2); 
      }
      else
      {
        unsigned int outputChannels = 0;
        unsigned int outputFrameSize = 0;
        pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_CHANNEL_COUNT, &outputChannels); // Get output channels
        pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, &outputFrameSize); // Get output frame size
        workingDesc.GetAttributes()->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT, outputChannels); // Update to reflect conversion performed by filter
        workingDesc.GetAttributes()->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME, outputFrameSize); // Update to reflect conversion performed by filter
        workingDesc.GetAttributes()->Remove(MA_ATT_TYPE_CHANNEL_LAYOUT); // TODO: This is a bad way to handle this
      }
      m_pTail->filter->SetOutputFormat(&workingDesc);
      if (pPrev)
        m_pTail->filter->SetSource(pPrev->filter);
      if (!m_pHead)
        m_pHead = m_pTail;
      filterChainText += " -> MatrixMixer";
    }

    // Resample if neccessary
    unsigned int inputSampleRate = 0;
    unsigned int outputSampleRate = 0;
    workingDesc.GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &inputSampleRate);
    pOutDesc->GetAttributes()->GetUInt(MA_ATT_TYPE_SAMPLERATE, &outputSampleRate);
    if (inputSampleRate != outputSampleRate)
    {
      dsp_filter_node* pPrev = m_pTail;
      m_pTail = new dsp_filter_node();
      m_pTail->filter = new CDSPFilterResampler();
      m_pTail->filter->SetInputFormat(&workingDesc);
      workingDesc.GetAttributes()->SetUInt(MA_ATT_TYPE_SAMPLERATE, outputSampleRate); // Update to reflect conversion performed by filter
      m_pTail->filter->SetOutputFormat(&workingDesc);
      filterChainText += " -> Resampler";
      if (pPrev)
        m_pTail->filter->SetSource(pPrev->filter);
      if (!m_pHead)
        m_pHead = m_pTail;
    }
    
    // Encode if necessary
    if (outputFormat == MA_STREAM_FORMAT_IEC61937)
    {
      dsp_filter_node* pPrev = m_pTail;
      m_pTail = new dsp_filter_node();
      m_pTail->filter = new CDSPFilterAC3Encoder();
      m_pTail->filter->SetInputFormat(&workingDesc);
      m_pTail->filter->SetOutputFormat(pOutDesc);    
      filterChainText += " -> AC3Encoder";
      if (pPrev)
        m_pTail->filter->SetSource(pPrev->filter);
      if (!m_pHead)
        m_pHead = m_pTail;
    }
  }
  filterChainText += " -> Output";
  CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Created filter graph: %s", filterChainText.c_str());

  return MA_SUCCESS;
}

void CDSPChain::Close()
{
  DisposeGraph(); // Clean up any remaining filters

  CDSPFilter::Close(); // Call base class method
}

MA_RESULT CDSPChain::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  if (!pOutput)
    return MA_ERROR;

  if (bus >= m_OutputBusses)
    return MA_INVALID_BUS;

  if (m_pTail)
  {
    // Pull data through the graph
    return m_pTail->filter->Render(pOutput, frameCount, renderTime, renderFlags, bus);
  }

  // Maybe we are just passing data through (no filters in the graph). Use the default implementation and pass the data through
  return CDSPFilter::Render(pOutput, frameCount, renderTime, renderFlags, bus);
}

float CDSPChain::GetDelay()
{
  // TODO: Calculate and return the actual latency through the filter graph
  return 0.0f;
}

MA_RESULT CDSPChain::SetSource(IAudioSource* pSource, unsigned int sourceBus /* = 0*/, unsigned int sinkBus /* = 0*/)
{
  if (m_pHead)
    return m_pHead->filter->SetSource(pSource, sourceBus, sinkBus);

  return CDSPFilter::SetSource(pSource, sourceBus, sinkBus);
}

void CDSPChain::Flush()
{
  // TODO: Implement
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