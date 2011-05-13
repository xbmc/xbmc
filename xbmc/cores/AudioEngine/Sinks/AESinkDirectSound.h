#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AESink.h"
#include <stdint.h>
#include <dsound.h>

#include "threads/CriticalSection.h"

class CAESinkDirectSound : public IAESink
{
public:
  virtual const char *GetName() { return "DirectSound"; }

  CAESinkDirectSound();
  virtual ~CAESinkDirectSound();

  virtual bool Initialize  (AEAudioFormat &format, CStdString &device);
  virtual void Deinitialize();
  virtual bool IsCompatible(const AEAudioFormat format, const CStdString device);

  virtual void         Stop             ();
  virtual float        GetDelay         ();
  virtual unsigned int AddPackets       (uint8_t *data, unsigned int frames);
  static  void         EnumerateDevices (AEDeviceList &devices, bool passthrough);
private:
  void          AEChannelsFromSpeakerMask(DWORD speakers);
  DWORD         SpeakerMaskFromAEChannels(AEChLayout channels);
  void          CheckPlayStatus();
  void          UpdateCacheStatus();
  unsigned int  GetSpace();
  char         *dserr2str(int err);

  LPDIRECTSOUNDBUFFER m_pBuffer;
  LPDIRECTSOUND8      m_pDSound;

  AEAudioFormat       m_format;
  enum AEChannel      m_channelLayout[9];
  CStdString          m_device;

  unsigned int        m_AvgBytesPerSec;

  unsigned int        m_dwChunkSize;
  unsigned int        m_dwFrameSize;
  unsigned int        m_dwBufferLen;

  unsigned int        m_BufferOffset;
  unsigned int        m_CacheLen;
  unsigned int        m_LastCacheCheck;

  bool                m_running;
  bool                m_initialized;
  CCriticalSection    m_runLock;
};
