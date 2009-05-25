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

#ifndef __DSP_CHAIN_H__
#define __DSP_CHAIN_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include "Filters/DSPFilter.h"

struct dsp_filter_node
{
  IDSPFilter* filter;
  dsp_filter_node* prev;
  dsp_filter_node* next;
};

class CDSPChain : public CDSPFilter
{
public:
  CDSPChain();
  virtual ~CDSPChain();

  MA_RESULT CreateFilterGraph(CStreamDescriptor* pInDesc, CStreamDescriptor* pOutDesc);

  // Base class overrides
  MA_RESULT SetSource(IAudioSource* pSource, unsigned int sourceBus = 0, unsigned int sinkBus = 0);
  MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
  void Close();
  float GetMaxLatency();
  void Flush();

protected:
  void DisposeGraph();
  dsp_filter_node* m_pHead;
  dsp_filter_node* m_pTail;
  void Create(unsigned int inputBusses, unsigned int outputBusses);
  void Destroy();
};

#endif // __DSP_CHAIN_H__