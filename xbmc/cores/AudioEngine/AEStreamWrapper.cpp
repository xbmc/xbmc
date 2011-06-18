/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AEStreamWrapper.h"
#include "AEFactory.h"

CAEStreamWrapper::CAEStreamWrapper(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options) :
  m_dataFormat      (dataFormat   ),
  m_sampleRate      (sampleRate   ),
  m_channelCount    (channelCount ),
  m_channelLayout   (channelLayout),
  m_options         (options      ),

  m_stream          (NULL),
  m_volume          (1.0f),
  m_replayGain      (1.0f),
  m_resampleRatio   (1.0 ),
  m_streamRatio     (0.0 ),
  m_callback        (NULL),
  m_dataCallback    (NULL),
  m_dataCallbackArg (NULL),
  m_drainCallback   (NULL),
  m_drainCallbackArg(NULL),
  m_freeCallback    (NULL),
  m_freeCallbackArg (NULL)
{
  Initialize();
}

CAEStreamWrapper::~CAEStreamWrapper()
{
  AE.RemoveStreamWrapper(this);
  if (m_freeCallback)
    m_freeCallback(this, m_freeCallbackArg, 0);
}

void CAEStreamWrapper::UnInitialize()
{
  CExclusiveLock lock(m_lock);

  if (m_stream)
  {
    for(std::list<IAEPostProc*>::iterator itt = m_postproc.begin(); itt != m_postproc.end(); ++itt)
      m_stream->RemovePostProc(*itt);

    if (m_callback)
      m_stream->UnRegisterAudioCallback();

    m_stream->SetDataCallback (NULL, NULL);
    m_stream->SetDrainCallback(NULL, NULL);
    m_stream->SetFreeCallback (NULL, NULL);
  }

  m_stream = NULL;
}

void CAEStreamWrapper::Initialize()
{
  CExclusiveLock lock(m_lock);

  IAE* ae = AE.GetEngine();
  
  if(!ae)
    return;
  
  m_stream = AE.GetEngine()->GetStream(m_dataFormat, m_sampleRate, m_channelCount, m_channelLayout, m_options);
  if (m_stream)
  {
    m_stream->SetVolume       (m_volume       );
    m_stream->SetReplayGain   (m_replayGain   );

    m_streamRatio = m_stream->GetResampleRatio();
    m_stream->SetResampleRatio(m_streamRatio * m_resampleRatio);

    for(std::list<IAEPostProc*>::iterator itt = m_postproc.begin(); itt != m_postproc.end(); ++itt)
      m_stream->AppendPostProc(*itt);

    if (m_callback)
      m_stream->RegisterAudioCallback(m_callback);

    if (m_dataCallback)
      m_stream->SetDataCallback(StaticStreamOnData, this);

    if (m_drainCallback)
      m_stream->SetDrainCallback(StaticStreamOnDrain, this);

    /* we need to know when the stream is freed as we need to free too */
    m_stream->SetFreeCallback(StaticStreamOnFree, this);
  }
}

void CAEStreamWrapper::StaticStreamOnData(IAEStream *sender, void *arg, unsigned int needed)
{  
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;

  CSharedLock lock(s->m_lock);
  s->m_dataCallback(s, s->m_dataCallbackArg, needed);
}

void CAEStreamWrapper::StaticStreamOnDrain(IAEStream *sender, void *arg, unsigned int unused)
{
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;

  CSharedLock lock(s->m_lock);
  s->m_drainCallback(s, s->m_drainCallbackArg, unused);
}

void CAEStreamWrapper::StaticStreamOnFree(IAEStream *sender, void *arg, unsigned int unused)
{
  /* delete ourself */
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;
  delete s;
}

void CAEStreamWrapper::AlterStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  CExclusiveLock lock(m_lock);

  m_dataFormat    = dataFormat;
  m_sampleRate    = sampleRate;
  m_channelCount  = channelCount;
  m_channelLayout = channelLayout;
  m_options       = options;

  IAE* ae = AE.GetEngine();
  if (ae && m_stream)
    m_stream = ae->AlterStream(m_stream, dataFormat, sampleRate, channelCount, channelLayout, options);
}

void CAEStreamWrapper::FreeStream()
{
  CExclusiveLock lock(m_lock);

  IAE* ae = AE.GetEngine();
  if (ae && m_stream)
    m_stream = ae->FreeStream(m_stream);
}

void CAEStreamWrapper::Destroy()
{
  CSharedLock lock(m_lock);
  if (m_stream)
  {
    m_stream->Destroy();
    m_stream = NULL;
  }
}

void CAEStreamWrapper::DisableCallbacks(bool free /* = true */)
{
  CSharedLock lock(m_lock);
  m_stream->DisableCallbacks(free);
}

void CAEStreamWrapper::SetDataCallback(AECBFunc *cbFunc, void *arg)
{
  {
    CExclusiveLock lock(m_lock);
    m_dataCallback    = cbFunc;
    m_dataCallbackArg = arg;
  }

  CSharedLock lock(m_lock);
  if (m_stream)
  {
    if (m_dataCallback)
      m_stream->SetDataCallback(StaticStreamOnData, this);
    else
      m_stream->SetDataCallback(NULL, NULL);
  }
}

void CAEStreamWrapper::SetDrainCallback(AECBFunc *cbFunc, void *arg)
{
  {
    CExclusiveLock lock(m_lock);
    m_drainCallback    = cbFunc;
    m_drainCallbackArg = arg;
  }

  CSharedLock lock(m_lock);
  if (m_stream)
  {
    if (m_drainCallback)
      m_stream->SetDrainCallback(StaticStreamOnDrain, this);
    else
      m_stream->SetDrainCallback(NULL, NULL);
  }
}

void CAEStreamWrapper::SetFreeCallback(AECBFunc *cbFunc, void *arg)
{
  CExclusiveLock lock(m_lock);
  m_freeCallback    = cbFunc;
  m_freeCallbackArg = arg;
}

unsigned int CAEStreamWrapper::AddData(void *data, unsigned int size)
{
  CSharedLock lock(m_lock);
  unsigned int ret = size;
  if (m_stream)
    ret = m_stream->AddData(data, size);

  return ret;
}

float CAEStreamWrapper::GetDelay()
{
  CSharedLock lock(m_lock);
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetDelay();

  return ret;
}

float CAEStreamWrapper::GetCacheTime()
{
  CSharedLock lock(m_lock);
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetCacheTime();

  return ret;
}

float CAEStreamWrapper::GetCacheTotal()
{
  CSharedLock lock(m_lock);
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetCacheTotal();

  return ret;
}

void CAEStreamWrapper::Pause()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_stream->Pause();
}

void CAEStreamWrapper::Resume()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_stream->Resume();
}

void CAEStreamWrapper::Drain()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_stream->Drain();
}

bool CAEStreamWrapper::IsDraining()
{
  CSharedLock lock(m_lock);
  bool ret = false;
  if (m_stream)
    ret = m_stream->IsDraining();

  return ret;
}

void CAEStreamWrapper::Flush()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_stream->Flush();
}

float CAEStreamWrapper::GetVolume()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_volume = m_stream->GetVolume();

  return m_volume;
}

void CAEStreamWrapper::SetVolume(float volume)
{
  CSharedLock lock(m_lock);
  m_volume = volume;
  if (m_stream)
    m_stream->SetVolume(volume);
}

float CAEStreamWrapper::GetReplayGain()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_replayGain = m_stream->GetReplayGain();

  return m_replayGain;
}

void  CAEStreamWrapper::SetReplayGain(float factor)
{
  CSharedLock lock(m_lock);
  m_replayGain = factor;
  if (m_stream)
    m_stream->SetReplayGain(factor);
}

void CAEStreamWrapper::AppendPostProc(IAEPostProc *pp)
{
  CSharedLock lock(m_lock);

  m_postproc.push_front(pp);
  if (m_stream)
    m_stream->AppendPostProc(pp);

}

void CAEStreamWrapper::PrependPostProc(IAEPostProc *pp)
{
  CSharedLock lock(m_lock);

  m_postproc.push_back(pp);
  if (m_stream)
    m_stream->PrependPostProc(pp);

}

void CAEStreamWrapper::RemovePostProc(IAEPostProc *pp)
{
  CSharedLock lock(m_lock);

  for(std::list<IAEPostProc*>::iterator itt = m_postproc.begin(); itt != m_postproc.end(); ++itt)
    if (*itt == pp)
    {
      m_postproc.erase(itt);
      break;
    }

  if (m_stream)
    m_stream->RemovePostProc(pp);

}

unsigned int CAEStreamWrapper::GetFrameSize()
{
  CSharedLock lock(m_lock);
  unsigned int ret = 0;
  if (m_stream) 
    ret = m_stream->GetFrameSize();

  return ret;
}

unsigned int CAEStreamWrapper::GetChannelCount()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_channelCount = m_stream->GetChannelCount();

  return m_channelCount;
}

unsigned int CAEStreamWrapper::GetSampleRate()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_sampleRate = m_stream->GetSampleRate();

  return m_sampleRate;
}

enum AEDataFormat CAEStreamWrapper::GetDataFormat()
{
  CSharedLock lock(m_lock);
  if (m_stream)
    m_dataFormat = m_stream->GetDataFormat();

  return m_dataFormat;
}

double CAEStreamWrapper::GetResampleRatio()
{
  CSharedLock lock(m_lock);

  return m_streamRatio;
}

void CAEStreamWrapper::SetResampleRatio(double ratio)
{
  CSharedLock lock(m_lock);
  m_resampleRatio = ratio;
  if (m_stream)
    m_stream->SetResampleRatio(m_streamRatio * m_resampleRatio);
}

void CAEStreamWrapper::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSharedLock lock(m_lock);
  m_callback = pCallback;
  if (m_stream)
    m_stream->RegisterAudioCallback(pCallback);
}

void CAEStreamWrapper::UnRegisterAudioCallback()
{
  CSharedLock lock(m_lock);
  m_callback = NULL;
  if (m_stream)
    m_stream->UnRegisterAudioCallback();
}

