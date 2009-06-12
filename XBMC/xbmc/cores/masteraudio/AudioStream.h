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

#ifndef __AUDIO_STREAM_H__
#define __AUDIO_STREAM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include "DSPChain.h"

class CAudioStream
{
public:
  CAudioStream();
  virtual ~CAudioStream();

  // Processing interface
  bool Render();

  // Stream interface
  bool Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, IRenderingAdapter* pRenderAdapter);
  void Destroy();
  unsigned int AddData(void* pData, unsigned int len);
  void Flush();
  bool Drain(unsigned int timeout);
  void SendCommand(int command);
  void SetLevel(float level);
  float GetDelay();

  // Temporary
  IRenderingAdapter* GetRenderingAdapter() {return m_pRenderingAdapter;}

private:
  CStreamInput* m_pInput; // We are responsible for cleaning this up
  CDSPChain* m_pDSPChain; // We are responsible for cleaning this up
  IRenderingAdapter* m_pRenderingAdapter;
  lap_timer m_ProcessTimer;
  lap_timer m_IntervalTimer;
  bool m_Open;
};
#endif // __AUDIO_STREAM_H__