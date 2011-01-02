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

#include "system.h"
#ifdef HAS_PULSEAUDIO

#include "PulseAESound.h"
#include "AEFactory.h"
#include "utils/log.h"
#include "MathUtils.h"
#include "StringUtils.h"

CPulseAESound::CPulseAESound(const CStdString &filename, pa_context *context, pa_threaded_mainloop *mainLoop) :
  IAESound  (filename),
  m_filename(filename),
  m_dataSent(0       ),
  m_context (context ),
  m_mainLoop(mainLoop),
  m_stream  (NULL    )
{
  m_pulseName = StringUtils::CreateUUID();
}

CPulseAESound::~CPulseAESound()
{
  DeInitialize();
}

bool CPulseAESound::Initialize()
{
  /* we dont re-init the wav loader in PA as PA handles the samplerate */
  if (!m_wavLoader.IsValid() && !m_wavLoader.Initialize(m_filename, 0))
    return false;

  m_sampleSpec.format   = PA_SAMPLE_FLOAT32NE;
  m_sampleSpec.rate     = m_wavLoader.GetSampleRate();
  m_sampleSpec.channels = m_wavLoader.GetChannelCount();

  if (!pa_sample_spec_valid(&m_sampleSpec))
  {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Invalid sample spec");
    return false;
  }

  struct pa_channel_map map;
  map.channels = m_sampleSpec.channels;
  switch(map.channels) {
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

  m_maxVolume     = AE.GetVolume();
  m_volume        = 1.0f;
  float useVolume = m_volume * m_maxVolume;

  pa_volume_t paVolume = MathUtils::round_int(useVolume * PA_VOLUME_NORM);
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

  if (pa_stream_connect_upload(m_stream, m_wavLoader.GetFrameCount() * pa_frame_size(&m_sampleSpec)) != 0) {
    CLog::Log(LOGERROR, "CPulseAESound::Initialize - Could not initialize the stream");
    pa_stream_disconnect(m_stream);
    m_stream = NULL;
    pa_threaded_mainloop_unlock(m_mainLoop);
    return false;
  }

  /* Wait until the stream is ready */
  while (pa_stream_get_state(m_stream) != PA_STREAM_READY && pa_stream_get_state(m_stream) != PA_STREAM_FAILED)
    pa_threaded_mainloop_wait(m_mainLoop);

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
  m_wavLoader.DeInitialize();
  if (m_stream) {
    pa_stream_disconnect(m_stream);
    m_stream = NULL;
  }
}

void CPulseAESound::Play()
{
  pa_threaded_mainloop_lock(m_mainLoop);
  pa_operation *o = pa_context_play_sample(m_context, m_pulseName.c_str(), NULL, PA_VOLUME_INVALID, NULL, NULL);
  if (o)
    pa_operation_unref(o);
  pa_threaded_mainloop_unlock(m_mainLoop);
}

void CPulseAESound::Stop()
{
}

bool CPulseAESound::IsPlaying()
{
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
  if (left == send) {
    pa_stream_set_write_callback(m_stream, NULL, NULL);
    if (pa_stream_finish_upload(m_stream) != 0)
    {
      CLog::Log(LOGERROR, "CPulseAESound::Upload - Error occured");
      /* FIXME: Better error handling */
    }
  }
}

#endif
