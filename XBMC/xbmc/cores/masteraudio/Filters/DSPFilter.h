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
  CDSPFilter();
  CDSPFilter(unsigned int inputBusses, unsigned int outputBusses);
  virtual ~CDSPFilter();

  // IAudioSink
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetSource(IAudioSource* pSource, unsigned int sourceBus, unsigned int sinkBus = 0);
  virtual float GetMaxLatency(); // TODO: This is the wrong place for this
  virtual void Flush(); // TODO: This is the wrong place for this

  // IAudioSource
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);

  // IDSPFilter
  virtual void Close();

protected:
  struct StreamAttributes
  {
  // Required attributes for all stream descriptor
    bool m_Locked;                 //MA_ATT_TYPE_STREAM_FLAGS
    bool m_VariableBitrate;        //MA_ATT_TYPE_STREAM_FLAGS
    unsigned int m_BytesPerSecond; //MA_ATT_TYPE_BYTES_PER_SEC
    unsigned int m_FrameSize;      //MA_ATT_TYPE_BYTES_PER_FRAME
    int m_StreamFormat;            //MA_ATT_TYPE_STREAM_FORMAT
  };

  // Internal member to be called by derived classes (not to be overridden)
  ma_audio_container* GetInputData(unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);

  // Internal member to be overridden by derived classes
  virtual MA_RESULT RenderOutput(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
  
  void ClearInputFormat(unsigned int bus = 0);
  void ClearOutputFormat(unsigned int bus = 0);

  unsigned int m_InputBusses;
  unsigned int m_OutputBusses;

  StreamAttributes* m_pInputDescriptor;
  StreamAttributes* m_pOutputDescriptor;

private:
  struct InputBus
  {
    IAudioSource* source;
    unsigned int bus;
  };

  void Create(unsigned int inputBusses, unsigned int outputBusses);
  void Destroy();

  InputBus* m_pInput;
  ma_audio_container** m_pInputContainer;

};

#endif // __DSP_FILTER_H__