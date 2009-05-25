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

  if (SetInputFormat(pInDesc, 0) != MA_SUCCESS || 
      SetOutputFormat(pOutDesc, 0) != MA_SUCCESS)
    return MA_NOT_SUPPORTED;

  // See if the stream wants to be passed-through untouched (must be specified in both input AND output descriptors)
  if (GetInputAttributes(0)->m_Locked && GetOutputAttributes(0)->m_Locked)
  {
    CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Detected locked stream. No filters will be created or applied.");
    // TODO: How do we want to handle scenarios where the flag cannot be honored because it is not set on both input and output
  }
 
  // TODO: Dynamically create DSP Filter Graph 

  dsp_filter_node* pNode = new dsp_filter_node();
  pNode->filter = new CDSPFilterMatrixMixer();
  pNode->filter->SetInputFormat(pInDesc);
  pNode->filter->SetOutputFormat(pOutDesc);
  m_pHead = m_pTail = pNode;

  CLog::Log(LOGINFO,"MasterAudio:CDSPChain: Creating dummy filter graph.");

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

float CDSPChain::GetMaxLatency()
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

  CDSPFilter::Flush();
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