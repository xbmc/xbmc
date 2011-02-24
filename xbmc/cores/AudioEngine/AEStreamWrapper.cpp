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
  m_resampleRatio   (1.0f),
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
  m_lock.EnterExclusive();

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
  m_lock.LeaveExclusive();
}

void CAEStreamWrapper::Initialize()
{
  m_lock.EnterExclusive();

  m_stream = AE.GetEngine()->GetStream(m_dataFormat, m_sampleRate, m_channelCount, m_channelLayout, m_options);
  if (m_stream)
  {
    m_stream->SetVolume       (m_volume       );
    m_stream->SetReplayGain   (m_replayGain   );
    m_stream->SetResampleRatio(m_resampleRatio);

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

  m_lock.LeaveExclusive();
}

void CAEStreamWrapper::StaticStreamOnData(IAEStream *sender, void *arg, unsigned int needed)
{
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;
  s->m_lock.EnterShared();  
  s->m_dataCallback(s, s->m_dataCallbackArg, needed);
  s->m_lock.LeaveShared();
}

void CAEStreamWrapper::StaticStreamOnDrain(IAEStream *sender, void *arg, unsigned int unused)
{
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;
  s->m_lock.EnterShared();
  s->m_drainCallback(s, s->m_drainCallbackArg, unused);
  s->m_lock.LeaveShared();
}

void CAEStreamWrapper::StaticStreamOnFree(IAEStream *sender, void *arg, unsigned int unused)
{
  /* delete ourself */
  CAEStreamWrapper *s = (CAEStreamWrapper*)arg;
  delete s;
}

void CAEStreamWrapper::AlterStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options)
{
  m_lock.EnterExclusive();

  m_dataFormat    = dataFormat;
  m_sampleRate    = sampleRate;
  m_channelCount  = channelCount;
  m_channelLayout = channelLayout;
  m_options       = options;

  IAE* ae = AE.GetEngine();
  if (m_stream)
    m_stream = ae->AlterStream(m_stream, dataFormat, sampleRate, channelCount, channelLayout, options);

  m_lock.LeaveExclusive();
}

void CAEStreamWrapper::Destroy()
{
  m_lock.EnterShared();
  if (m_stream)
  {
    m_stream->Destroy();
    m_stream = NULL;
  }
  m_lock.LeaveShared();
}

void CAEStreamWrapper::SetDataCallback(AECBFunc *cbFunc, void *arg)
{
  m_lock.EnterShared();
  m_dataCallback    = cbFunc;
  m_dataCallbackArg = arg;
  if (m_stream)
  {
    if (m_dataCallback)
      m_stream->SetDataCallback(StaticStreamOnData, this);
    else
      m_stream->SetDataCallback(NULL, NULL);
  }
  m_lock.LeaveShared();
}

void CAEStreamWrapper::SetDrainCallback(AECBFunc *cbFunc, void *arg)
{
  m_lock.EnterShared();
  m_drainCallback    = cbFunc;
  m_drainCallbackArg = arg;
  if (m_stream)
  {
    if (m_drainCallback)
      m_stream->SetDrainCallback(StaticStreamOnDrain, this);
    else
      m_stream->SetDrainCallback(NULL, NULL);
  }
  m_lock.LeaveShared();
}

void CAEStreamWrapper::SetFreeCallback(AECBFunc *cbFunc, void *arg)
{
  m_lock.EnterShared();
  m_freeCallback    = cbFunc;
  m_freeCallbackArg = arg;
  m_lock.LeaveShared();
}

unsigned int CAEStreamWrapper::AddData(void *data, unsigned int size)
{
  m_lock.EnterShared();
  unsigned int ret = size;
  if (m_stream)
    ret = m_stream->AddData(data, size);
  m_lock.LeaveShared();

  return ret;
}

float CAEStreamWrapper::GetDelay()
{
  m_lock.EnterShared();
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetDelay();
  m_lock.LeaveShared();

  return ret;
}

float CAEStreamWrapper::GetCacheTime()
{
  m_lock.EnterShared();
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetCacheTime();
  m_lock.LeaveShared();

  return ret;
}

float CAEStreamWrapper::GetCacheTotal()
{
  m_lock.EnterShared();
  float ret = 0;
  if (m_stream)
    ret = m_stream->GetCacheTotal();
  m_lock.LeaveShared();

  return ret;
}

void CAEStreamWrapper::Pause()
{
  m_lock.EnterShared();
  if (m_stream)
    m_stream->Pause();
  m_lock.LeaveShared();
}

void CAEStreamWrapper::Resume()
{
  m_lock.EnterShared();
  if (m_stream)
    m_stream->Resume();
  m_lock.LeaveShared();
}

void CAEStreamWrapper::Drain()
{
  m_lock.EnterShared();
  if (m_stream)
    m_stream->Drain();
  m_lock.LeaveShared();
}

bool CAEStreamWrapper::IsDraining()
{
  m_lock.EnterShared();
  bool ret = false;
  if (m_stream)
    ret = m_stream->IsDraining();
  m_lock.LeaveShared();

  return ret;
}

void CAEStreamWrapper::Flush()
{
  m_lock.EnterShared();
  if (m_stream)
    m_stream->Flush();
  m_lock.LeaveShared();
}

float CAEStreamWrapper::GetVolume()
{
  m_lock.EnterShared();
  if (m_stream)
    m_volume = m_stream->GetVolume();
  m_lock.LeaveShared();

  return m_volume;
}

void CAEStreamWrapper::SetVolume(float volume)
{
  m_lock.EnterShared();
  m_volume = volume;
  if (m_stream)
    m_stream->SetVolume(volume);
  m_lock.LeaveShared();
}

float CAEStreamWrapper::GetReplayGain()
{
  m_lock.EnterShared();
  if (m_stream)
    m_replayGain = m_stream->GetReplayGain();
  m_lock.LeaveShared();

  return m_replayGain;
}

void  CAEStreamWrapper::SetReplayGain(float factor)
{
  m_lock.EnterShared();
  m_replayGain = factor;
  if (m_stream)
    m_stream->SetReplayGain(factor);
  m_lock.LeaveShared();
}

void CAEStreamWrapper::AppendPostProc(IAEPostProc *pp)
{
  m_lock.EnterShared();

  m_postproc.push_front(pp);
  if (m_stream)
    m_stream->AppendPostProc(pp);

  m_lock.LeaveShared();
}

void CAEStreamWrapper::PrependPostProc(IAEPostProc *pp)
{
  m_lock.EnterShared();

  m_postproc.push_back(pp);
  if (m_stream)
    m_stream->PrependPostProc(pp);

  m_lock.LeaveShared();
}

void CAEStreamWrapper::RemovePostProc(IAEPostProc *pp)
{
  m_lock.EnterShared();

  for(std::list<IAEPostProc*>::iterator itt = m_postproc.begin(); itt != m_postproc.end(); ++itt)
    if (*itt == pp)
    {
      m_postproc.erase(itt);
      break;
    }

  if (m_stream)
    m_stream->RemovePostProc(pp);

  m_lock.LeaveShared();
}

unsigned int CAEStreamWrapper::GetFrameSize()
{
  m_lock.EnterShared();
  unsigned int ret = 0;
  if (m_stream) 
    ret = m_stream->GetFrameSize();
  m_lock.LeaveShared();

  return ret;
}

unsigned int CAEStreamWrapper::GetChannelCount()
{
  m_lock.EnterShared();
  if (m_stream)
    m_channelCount = m_stream->GetChannelCount();
  m_lock.LeaveShared();

  return m_channelCount;
}

unsigned int CAEStreamWrapper::GetSampleRate()
{
  m_lock.EnterShared();
  if (m_stream)
    m_sampleRate = m_stream->GetSampleRate();
  m_lock.LeaveShared();

  return m_sampleRate;
}

enum AEDataFormat CAEStreamWrapper::GetDataFormat()
{
  m_lock.EnterShared();
  if (m_stream)
    m_dataFormat = m_stream->GetDataFormat();
  m_lock.LeaveShared();

  return m_dataFormat;
}

double CAEStreamWrapper::GetResampleRatio()
{
  m_lock.EnterShared();
  if (m_stream)
    m_resampleRatio = m_stream->GetResampleRatio();
  m_lock.LeaveShared();

  return m_resampleRatio;
}

void CAEStreamWrapper::SetResampleRatio(double ratio)
{
  m_lock.EnterShared();
  m_resampleRatio = ratio;
  if (m_stream)
    m_stream->SetResampleRatio(ratio);
  m_lock.LeaveShared();
}

void CAEStreamWrapper::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_lock.EnterShared();
  m_callback = pCallback;
  if (m_stream)
    m_stream->RegisterAudioCallback(pCallback);
  m_lock.LeaveShared();
}

void CAEStreamWrapper::UnRegisterAudioCallback()
{
  m_lock.EnterShared();
  m_callback = NULL;
  if (m_stream);
    m_stream->UnRegisterAudioCallback();
  m_lock.LeaveShared();
}

