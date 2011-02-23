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
#include "AESoundWrapper.h"

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
     /* unload any streams */
    for(std::list<CAEStreamWrapper*>::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
      (*itt)->UnInitialize();
   
    /* unload any sounds */
    for(std::list<CAESoundWrapper*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
      (*itt)->UnLoad();

    delete m_ae;
    m_ae = NULL;
  }

  if (ae && ae->Initialize())
      m_ae = ae;

  /* reload any streams */
  for(std::list<CAEStreamWrapper*>::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->Initialize();

  /* reload any sounds */
  for(std::list<CAESoundWrapper*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    (*itt)->Load();
  
  m_lock.LeaveExclusive();  
  return (m_ae != NULL);
}

IAE* CAEWrapper::GetEngine()
{
   return m_ae;
}

void CAEWrapper::RemoveStreamWrapper(CAEStreamWrapper *wrapper)
{
  for(std::list<CAEStreamWrapper*>::iterator itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    if (*itt == wrapper)
    {
      m_streams.erase(itt);
      break;
    }
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
  m_lock.EnterShared();
  CAEStreamWrapper *wrapper = new CAEStreamWrapper(dataFormat, sampleRate, channelCount, channelLayout, options);
  m_streams.push_back(wrapper);
  m_lock.LeaveShared();

  return wrapper;
}

IAEStream* CAEWrapper::AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  m_lock.EnterShared();
  CAEStreamWrapper *wrapper = (CAEStreamWrapper*)stream;
  wrapper->AlterStream(dataFormat, sampleRate, channelCount, channelLayout, options);
  m_lock.LeaveShared();
  return wrapper;
}

IAESound* CAEWrapper::GetSound(CStdString file)
{
  CAESoundWrapper *s = NULL;

  m_lock.EnterShared();
  s = new CAESoundWrapper(file);
  m_sounds.push_back(s);
  m_lock.LeaveShared();

  return s;
}

void CAEWrapper::FreeSound(IAESound *sound)
{
  m_lock.EnterShared();

  ((CAESoundWrapper*)sound)->UnLoad();
  for(std::list<CAESoundWrapper*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }
  delete (CAESoundWrapper*)sound;

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

