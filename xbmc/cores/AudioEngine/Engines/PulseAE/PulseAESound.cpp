/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifdef HAS_PULSEAUDIO

#include "PulseAESound.h"
#include "AEFactory.h"
#include "utils/log.h"
#include "MathUtils.h"
#include "StringUtils.h"

CPulseAESound::CPulseAESound(const std::string &filename, pa_context *context, pa_threaded_mainloop *mainLoop) :
  IAESound         (filename),
  m_filename       (filename),
  m_dataSent       (0       ),
  m_context        (context ),
  m_mainLoop       (mainLoop),
  m_stream         (NULL    ),
  m_op             (NULL    )
{
  m_pulseName = StringUtils::CreateUUID();
  m_wavLoader.Load(filename);
}

CPulseAESound::~CPulseAESound()
{
  DeInitialize();
}

bool CPulseAESound::Initialize()
{
  /* we dont re-init the wav loader in PA as PA handles the samplerate */
  if (!m_wavLoader.IsValid())
    return false;

  m_sampleSpec.format   = PA_SAMPLE_FLOAT32NE;
  m_sampleSpec.rate     = m_wavLoader.GetSampleRate();
  m_sampleSpec.channels = m_wavLoader.GetChannelLayout().Count();

  if (!pa_sample_spec_valid(&m_sampleSpec))
  {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Invalid sample spec");
    return false;
  }

  struct pa_channel_map map;
  map.channels = m_sampleSpec.channels;
  switch (map.channels)
  {
    case 1:
      map.map[0] = PA_CHANNEL_POSITION_MONO;
      break;

    case 2:
      map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
      map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
      break;

    default:
      CLog::Log(LOGERROR, "CPulseAESound::Initialize - We do not yet support multichannel sounds");
      return false;
  }

  m_maxVolume     = CAEFactory::GetEngine()->GetVolume();
  m_volume        = 1.0f;
  pa_volume_t paVolume = pa_sw_volume_from_linear((double)(m_volume * m_maxVolume));
  pa_cvolume_set(&m_chVolume, m_sampleSpec.channels, paVolume);

  pa_threaded_mainloop_lock(m_mainLoop);
  if ((m_stream = pa_stream_new(m_context, m_pulseName.c_str(), &m_sampleSpec, &map)) == NULL)
  {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Could not create a stream");
    pa_threaded_mainloop_unlock(m_mainLoop);
    return false;
  }

  pa_stream_set_state_callback(m_stream, CPulseAESound::StreamStateCallback, this);
  pa_stream_set_write_callback(m_stream, CPulseAESound::StreamWriteCallback, this);

  if (pa_stream_connect_upload(m_stream, m_wavLoader.GetFrameCount() * pa_frame_size(&m_sampleSpec)) != 0)
  {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Could not initialize the stream");
    pa_stream_disconnect(m_stream);
    m_stream = NULL;
    pa_threaded_mainloop_unlock(m_mainLoop);
    return false;
  }

  /* check if the stream failed */
  if (pa_stream_get_state(m_stream) == PA_STREAM_FAILED)
  {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Waited for the stream but it failed");
    pa_stream_disconnect(m_stream);
    m_stream = NULL;
    pa_threaded_mainloop_unlock(m_mainLoop);
    return false;
  }

  pa_threaded_mainloop_unlock(m_mainLoop);
  return true;
}

void CPulseAESound::DeInitialize()
{
  pa_threaded_mainloop_lock(m_mainLoop);
  pa_operation *op = pa_context_remove_sample(m_context, m_pulseName.c_str(), NULL, NULL);
  if (op)
    pa_operation_unref(op);
  pa_threaded_mainloop_unlock(m_mainLoop);

  m_wavLoader.DeInitialize();
}

void CPulseAESound::Play()
{
  pa_threaded_mainloop_lock(m_mainLoop);
  /* we only keep the most recent operation as it is the only one needed for IsPlaying to function */
  if (m_op)
    pa_operation_unref(m_op);
  m_op = pa_context_play_sample(m_context, m_pulseName.c_str(), NULL, PA_VOLUME_INVALID, NULL, NULL);
  pa_threaded_mainloop_unlock(m_mainLoop);
}

void CPulseAESound::Stop()
{
  if (m_op)
  {
    pa_operation_cancel(m_op);
    pa_operation_unref(m_op);
    m_op = NULL;
  }
}

bool CPulseAESound::IsPlaying()
{
  if (m_op)
  {
    if (pa_operation_get_state(m_op) == PA_OPERATION_RUNNING)
      return true;

    pa_operation_unref(m_op);
    m_op = NULL;
  }

  return false;
}

void CPulseAESound::SetVolume(float volume)
{
}

float CPulseAESound::GetVolume()
{
  return 1.0f;
}

void CPulseAESound::StreamStateCallback(pa_stream *s, void *userdata)
{
  CPulseAESound *sound = (CPulseAESound*)userdata;
  pa_stream_state_t state = pa_stream_get_state(s);

  switch (state)
  {
    case PA_STREAM_FAILED:
      CLog::Log(LOGERROR, "CPulseAESound::StreamStateCallback - %s", pa_strerror(pa_context_errno(sound->m_context)));

    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
    case PA_STREAM_READY:
    case PA_STREAM_TERMINATED:
      pa_threaded_mainloop_signal(sound->m_mainLoop, 0);
      break;
  }
}

void CPulseAESound::StreamWriteCallback(pa_stream *s, size_t length, void *userdata)
{
  CPulseAESound *sound = (CPulseAESound*)userdata;
  sound->Upload(length);
}

void CPulseAESound::Upload(size_t length)
{
  float *samples = m_wavLoader.GetSamples();
  size_t left    = (m_wavLoader.GetSampleCount() * sizeof(float)) - m_dataSent;
  size_t send    = std::min(length, left);

  if (pa_stream_write(m_stream, samples + m_dataSent, send, 0, 0, PA_SEEK_RELATIVE) == 0)
    m_dataSent += send;

  /* if we have no more data disable the callback */
  if (left == send)
  {
    pa_stream_set_write_callback(m_stream, NULL, NULL);
    if (pa_stream_finish_upload(m_stream) != 0)
    {
      CLog::Log(LOGERROR, "CPulseAESound::Upload - Error occured");
      /* FIXME: Better error handling */
    }

    /* disconnect the stream as we dont need it anymore */
    pa_stream_disconnect(m_stream);
    m_stream = NULL;
  }
}

#endif
