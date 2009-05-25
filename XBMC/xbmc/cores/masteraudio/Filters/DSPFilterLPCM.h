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

#ifndef __DSP_FILTER_LPCM_H__
#define __DSP_FILTER_LPCM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DSPFilter.h"

// Base class for pure LPCM(In and Out) DSPFilters
class CDSPFilterLPCM : public CDSPFilter
{
public:
  CDSPFilterLPCM(unsigned int inputBusses = 1, unsigned int outputBusses = 1);
  virtual ~CDSPFilterLPCM();

  // IAudioSink
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);

  // IAudioSource
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);

protected:
  struct LPCMAttributes
  {
    bool         m_IsInterleaved;  //MA_ATT_TYPE_LPCM_FLAGS
    int          m_SampleType;     //MA_ATT_TYPE_SAMPLE_TYPE
    unsigned int m_BitDepth;       //MA_ATT_TYPE_BITDEPTH
    unsigned int m_SampleRate;     //MA_ATT_TYPE_SAMPLERATE
    unsigned int m_ChannelCount;   //MA_ATT_TYPE_CHANNEL_COUNT
  };

  inline const LPCMAttributes* GetLPCMInputAttributes(unsigned int bus = 0)
  {
    if (bus < m_InputBusses)
      return &m_pLPCMInputDescriptor[bus];
    return NULL;
  }

  inline const LPCMAttributes* GetLPCMOutputAttributes(unsigned int bus = 0)
  {
    if (bus < m_OutputBusses)
      return &m_pLPCMOutputDescriptor[bus];
    return NULL;
  }

  virtual void ClearInputFormat(unsigned int bus = 0);
  virtual void ClearOutputFormat(unsigned int bus = 0);
  
private:
  LPCMAttributes* m_pLPCMInputDescriptor;
  LPCMAttributes* m_pLPCMOutputDescriptor;
};

#endif // __DSP_FILTER_LPCM_H__