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

#ifndef __DSPFILTER_RESAMPLER_H__
#define __DSPFILTER_RESAMPLER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DSPFilterLPCM.h"
#include <samplerate.h>

class CDSPFilterResampler : public CDSPFilterLPCM
{
public:
  CDSPFilterResampler();
  virtual ~CDSPFilterResampler();
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
protected:
  SRC_STATE* m_Converter;
  ma_audio_container* m_pInputContainer;
  float* m_pInputData;
  unsigned int m_InputDataFrames;
  float* m_pOutputData;
  unsigned int m_OutputDataFrames;

  unsigned int m_RemainingFrames;

  double m_Ratio;

  MA_RESULT Init();
  void CleanUp();
  MA_RESULT GetFrames(int* pOutputFrames, float* pOutputData, unsigned int channels, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);
};

#endif // __DSPFILTER_RESAMPLER_H__