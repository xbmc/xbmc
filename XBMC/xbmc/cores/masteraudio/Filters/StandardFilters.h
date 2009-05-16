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

#ifndef __STANDARD_AUDIO_FILTERS_H__
#define __STANDARD_AUDIO_FILTERS_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../MasterAudioCore.h"

class CChannelRemapFilter : public IDSPFilter
{
public:
  CChannelRemapFilter();
  virtual ~CChannelRemapFilter();

  // IDSPFilter
  void Close();

  // IAudioSink
  MA_RESULT TestInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetInputProperties(audio_data_transfer_props* pProps);
  MA_RESULT AddSlice(audio_slice* pSlice);
  float GetMaxLatency();
  void Flush();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetOutputProperties(audio_data_transfer_props* pProps);
  MA_RESULT GetSlice(audio_slice** ppSlice);
protected:
  audio_slice* m_pSlice;
  __int64 m_InputMap;
  __int64 m_OutputMap;
  int m_Channels;
  int m_BitDepth;
  unsigned char* m_pWorkBuffer;
  void RemapChannels(audio_slice* pSlice);
};

#endif // __STANDARD_AUDIO_FILTERS_H__