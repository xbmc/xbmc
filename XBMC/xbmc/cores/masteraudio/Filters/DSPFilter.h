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

#ifndef __DSP_FILTER_H__
#define __DSP_FILTER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../MasterAudioCore.h"

// Base class for DSPFilters
class CDSPFilter : public IDSPFilter
{
public:
  CDSPFilter(unsigned int inputBusses = 1, unsigned int outputBusses = 1);
  virtual ~CDSPFilter();

  // IAudioSink
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetSource(IAudioSource* pSource, unsigned int sourceBus = 0, unsigned int sinkBus = 0);

  // IAudioSource
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
  virtual float GetDelay();

  // IDSPFilter
  virtual void Close();

protected:
  // Required attributes for all stream descriptor
  struct StreamAttributes
  {
    bool m_Locked;                 //MA_ATT_TYPE_STREAM_FLAGS
    bool m_VariableBitrate;        //MA_ATT_TYPE_STREAM_FLAGS
    unsigned int m_BytesPerSecond; //MA_ATT_TYPE_BYTES_PER_SEC
    unsigned int m_FrameSize;      //MA_ATT_TYPE_BYTES_PER_FRAME
    int m_StreamFormat;            //MA_ATT_TYPE_STREAM_FORMAT
  };

  MA_RESULT GetInputData(ma_audio_container* pInput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
  
  inline const StreamAttributes* GetInputAttributes(unsigned int bus = 0)
  {
    if (bus < m_InputBusses)
      return &m_pInputDescriptor[bus];
    return NULL;
  };

  inline const StreamAttributes* GetOutputAttributes(unsigned int bus = 0)
  {
    if (bus < m_InputBusses)
      return &m_pOutputDescriptor[bus];
    return NULL;
  };

  virtual void ClearInputFormat(unsigned int bus = 0);
  virtual void ClearOutputFormat(unsigned int bus = 0);

  unsigned int m_InputBusses;
  unsigned int m_OutputBusses;

private:
  struct InputBus
  {
    IAudioSource* source;
    unsigned int bus;
  };

  StreamAttributes* m_pInputDescriptor;
  StreamAttributes* m_pOutputDescriptor;
  InputBus* m_pInput;
};

#endif // __DSP_FILTER_H__