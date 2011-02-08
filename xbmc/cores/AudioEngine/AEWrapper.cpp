/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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


#include "AEWrapper.h"

CAEWrapper AE;

CAEWrapper::CAEWrapper() :
  m_ae(NULL)
{
}


CAEWrapper::~CAEWrapper()
{
}

bool CAEWrapper::Initialize()
{
  /* this really should never be called */
  return false;
}

bool CAEWrapper::SetEngine(IAE *ae)
{
  m_lock.EnterExclusive();

  /* shutdown the old engine */
  if (m_ae)
  {
    delete m_ae;
    m_ae = NULL;
  }

  if (ae && ae->Initialize())
      m_ae = ae;
  
  m_lock.LeaveExclusive();  
  return (m_ae != NULL);
}

IAE* CAEWrapper::GetEngine()
{
   return m_ae;
}

void CAEWrapper::OnSettingsChange(CStdString setting)
{
  m_lock.EnterShared();
  if (m_ae) m_ae->OnSettingsChange(setting);
  m_lock.LeaveShared();
}

float CAEWrapper::GetVolume()
{
  float vol = 0;
  m_lock.EnterShared();
  if (m_ae) vol = m_ae->GetVolume();
  m_lock.LeaveShared();

  return vol;
}

void CAEWrapper::SetVolume(float volume)
{
  m_lock.EnterShared();
  if (m_ae) m_ae->SetVolume(volume);
  m_lock.LeaveShared();
}

IAEStream* CAEWrapper::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  IAEStream *s = NULL;
  m_lock.EnterShared();
  if (m_ae) s = m_ae->GetStream(dataFormat, sampleRate, channelCount, channelLayout, options);
  m_lock.LeaveShared();

  return s;
}

IAEStream* CAEWrapper::AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  IAEStream *s = NULL;
  m_lock.EnterShared();
  if (m_ae) s = m_ae->AlterStream(stream, dataFormat, sampleRate, channelCount, channelLayout, options);
  m_lock.LeaveShared();

  return s;
}

IAESound* CAEWrapper::GetSound(CStdString file)
{
  IAESound *s = NULL;
  m_lock.EnterShared();
  if (m_ae) s = m_ae->GetSound(file);
  m_lock.LeaveShared();

  return s;
}

void CAEWrapper::FreeSound(IAESound *sound)
{
  m_lock.EnterShared();
  if (m_ae) m_ae->FreeSound(sound);
  m_lock.LeaveShared();
}

void CAEWrapper::GarbageCollect()
{
  m_lock.EnterShared();
  if (m_ae) m_ae->GarbageCollect();
  m_lock.LeaveShared();
}

void CAEWrapper::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  m_lock.EnterShared();
  if (m_ae) m_ae->EnumerateOutputDevices(devices, passthrough);
  m_lock.LeaveShared();
}

bool CAEWrapper::SupportsRaw()
{
  bool raw = false;
  m_lock.EnterShared();
  if (m_ae) raw = m_ae->SupportsRaw();
  m_lock.LeaveShared();

  return raw;
}

