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
  if(m_ae)
    delete m_ae;
}

bool CAEWrapper::Initialize()
{
  /* this really should never be called */
  return false;
}

bool CAEWrapper::SetEngine(IAE *ae)
{
  CExclusiveLock lock(m_lock);

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
  CSharedLock lock(m_lock);
  if (m_ae) m_ae->OnSettingsChange(setting);
}

float CAEWrapper::GetVolume()
{
  float vol = 0;
  CSharedLock lock(m_lock);
  if (m_ae) vol = m_ae->GetVolume();

  return vol;
}

void CAEWrapper::SetVolume(float volume)
{
  CSharedLock lock(m_lock);
  if (m_ae) m_ae->SetVolume(volume);
}

IAEStream* CAEWrapper::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  CSharedLock lock(m_lock);
  CAEStreamWrapper *wrapper = new CAEStreamWrapper(dataFormat, sampleRate, channelCount, channelLayout, options);
  m_streams.push_back(wrapper);

  return wrapper;
}

IAEStream* CAEWrapper::AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  CSharedLock lock(m_lock);
  CAEStreamWrapper *wrapper = (CAEStreamWrapper*)stream;
  wrapper->AlterStream(dataFormat, sampleRate, channelCount, channelLayout, options);
  return wrapper;
}

IAEStream *CAEWrapper::FreeStream(IAEStream *stream)
{
  CSharedLock lock(m_lock);
  CAEStreamWrapper *wrapper = (CAEStreamWrapper*)stream;
	wrapper->FreeStream();
  return wrapper;
}

IAESound* CAEWrapper::GetSound(CStdString file)
{
  CAESoundWrapper *s = NULL;

  CSharedLock lock(m_lock);
  s = new CAESoundWrapper(file);
  m_sounds.push_back(s);

  return s;
}

void CAEWrapper::FreeSound(IAESound *sound)
{
  CSharedLock lock(m_lock);

  ((CAESoundWrapper*)sound)->UnLoad();
  for(std::list<CAESoundWrapper*>::iterator itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
    if (*itt == sound)
    {
      m_sounds.erase(itt);
      break;
    }
  delete (CAESoundWrapper*)sound;

}

void CAEWrapper::GarbageCollect()
{
  CSharedLock lock(m_lock);
  if (m_ae) m_ae->GarbageCollect();
}

void CAEWrapper::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  CSharedLock lock(m_lock);
  if (m_ae) m_ae->EnumerateOutputDevices(devices, passthrough);
}

bool CAEWrapper::SupportsRaw()
{
  bool raw = false;
  CSharedLock lock(m_lock);
  if (m_ae) raw = m_ae->SupportsRaw();

  return raw;
}

