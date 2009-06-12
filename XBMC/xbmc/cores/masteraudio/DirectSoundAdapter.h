/*
 *      Copyright (C) 2009 Team XBMC
 *      http://www.xbmc.org
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

#ifndef __DIRECT_SOUND_ADAPTER_H__
#define __DIRECT_SOUND_ADAPTER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include "../AudioRenderers/AudioRendererFactory.h"

class CDirectSoundAdapter : public IRenderingAdapter
{
public:
  CDirectSoundAdapter();
  virtual ~CDirectSoundAdapter();

  // IAudioSink
  MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  MA_RESULT SetSource(IAudioSource* pSource, unsigned int sourceBus, unsigned int sinkBus = 0);
  float GetDelay();
  void Flush();

  // IRenderingControl
  void Play();
  void Stop();
  void Pause();
  void Resume();
  void SetVolume(long vol);

  // IRenderingAdapter
  void Close();
  bool Drain(unsigned int timeout);
  void Render();
  
protected:
  IAudioSource* m_pSource;
  unsigned int m_SourceBus;
  size_t m_ChunkLen;
  IAudioRenderer* m_pRenderer;
  unsigned __int64 m_TotalBytesReceived;
  unsigned int m_BytesPerFrame;
};

#endif // __DIRECT_SOUND_ADAPTER_H__