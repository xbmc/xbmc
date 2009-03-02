/*
 *      Copyright (C) 2009 phi2039
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

#ifndef __AUDIO_MANAGER_CLIENT_H__
#define __AUDIO_MANAGER_CLIENT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AudioManager.h"

#define TRY_CONTROL_STREAM(code)                \
  if (m_pManager)                               \
    m_pManager->ControlStream(m_StreamId, code)  \

#define TRY_SET_STREAM_PROP(prop, ptr)                        \
  if (m_pManager)                                             \
    return m_pManager->SetStreamProp(m_StreamId, prop, ptr);  \
  else                                                        \
    return false;                                             \

#define VALID_STREAM_ID(id) (id != MA_STREAM_NONE)

class CAudioManagerClient
{
public:
  CAudioManagerClient(CAudioManager* pManager);
  virtual ~CAudioManagerClient();

  // Control Interface
  virtual void Stop();
  virtual void Play();
  virtual void Pause();
  virtual void Resume();
  virtual bool SetVolume(long vol);

  // Stream Interface
  virtual void CloseStream();
  virtual int PutData(void* pData, size_t len);
  virtual float GetDelay();

  virtual bool IsStreamOpen() {return VALID_STREAM_ID(m_StreamId);}
  int AddDataToStream(void* pData, size_t len);

  virtual void DrainStream(int maxWaitTime);

protected:
  virtual bool OpenStream(CStreamDescriptor* pDesc, size_t blockSize);
  CAudioManager* m_pManager;
  AM_STREAM_ID GetStreamId() {return m_StreamId;}
private:
  AM_STREAM_ID m_StreamId;

};

#endif // __AUDIO_MANAGER_CLIENT_H__