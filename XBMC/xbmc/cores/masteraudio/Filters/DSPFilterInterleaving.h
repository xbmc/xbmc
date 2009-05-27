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

#ifndef __DSPFILTERINTERLEAVE_H__
#define __DSPFILTERINTERLEAVE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../MasterAudioCore.h"
#include "DSPFilterLPCM.h"

class CDSPFilterInterleaving : public CDSPFilterLPCM
{
public:

  CDSPFilterInterleaving(unsigned int inputBusses = 1, unsigned int outputBusses = 1);
  virtual ~CDSPFilterInterleaving();

  // IAudioSink
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);

  // IAudioSource
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);

protected:
private:
  enum
  {
    MODE_NONE,
    MODE_INTERLEAVE,
    MODE_DEINTERLEAVE
  };
  int m_Mode;
  ma_audio_container **m_pInputContainer;
  MA_RESULT CompareFormats(CStreamDescriptor* pDesc, const LPCMAttributes *pLPCM);
  MA_RESULT Interleave(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus);
  MA_RESULT DeInterleave(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus);
};

#endif // __DSPFILTERINTERLEAVE_H__
