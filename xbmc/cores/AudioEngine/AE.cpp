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
#include "utils/TimeUtils.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "GUISettings.h"
#include "Settings.h"

#if (defined USE_EXTERNAL_FFMPEG)
  #include <libavutil/avutil.h>
#else
  #include "cores/dvdplayer/Codecs/ffmpeg/libavutil/avutil.h"
#endif

#include "AE.h"
#include "AEUtil.h"
#include "AESink.h"
#include "Sinks/AESinkALSA.h"

#ifdef __SSE__
#include <xmmintrin.h>
#endif

using namespace std;

CAE::CAE():
  m_running      (false),
  m_reOpened     (false),
  m_sink         (NULL ),
  m_passthrough  (false),
  m_buffer       (NULL ),
  m_audioCallback(NULL )
{
}

CAE::~CAE()
{
  Deinitialize();

  /* free the streams */
  while(!m_streams.empty())
  {
    CAEStream *s = m_streams.front();
    /* note: the stream will call RemoveStream via it's dtor */
    delete s;
  }
}

bool CAE::OpenSink()
{
  /* lock the sink so the thread gets held up */
  CSingleLock sinkLock(m_critSectionSink);

  /* load the configuration */
  enum AEStdChLayout stdChLayout = AE_CH_LAYOUT_2_0;
  switch(g_guiSettings.GetInt("audiooutput.channellayout")) {
    default:
    case 0: stdChLayout = AE_CH_LAYOUT_2_0; break;
    case 1: stdChLayout = AE_CH_LAYOUT_2_1; break;
    case 2: stdChLayout = AE_CH_LAYOUT_3_0; break;
    case 3: stdChLayout = AE_CH_LAYOUT_3_1; break;
    case 4: stdChLayout = AE_CH_LAYOUT_4_0; break;
    case 5: stdChLayout = AE_CH_LAYOUT_4_1; break;
    case 6: stdChLayout = AE_CH_LAYOUT_5_0; break;
    case 7: stdChLayout = AE_CH_LAYOUT_5_1; break;
    case 8: stdChLayout = AE_CH_LAYOUT_7_0; break;
    case 9: stdChLayout = AE_CH_LAYOUT_7_1; break;
  }
  /* choose a sample rate based on the oldest stream, or if non, 44100hz */
  unsigned int sampleRate = 44100;
  if (!m_streams.empty())
    sampleRate = m_streams.front()->GetSampleRate();

  CStdString device = g_guiSettings.GetString("audiooutput.audiodevice");

  /* setup the desired format */
  AEAudioFormat desiredFormat;
  desiredFormat.m_channelLayout = CAEUtil::GetStdChLayout  (stdChLayout);
  desiredFormat.m_channelCount  = CAEUtil::GetChLayoutCount(desiredFormat.m_channelLayout);
  desiredFormat.m_sampleRate    = sampleRate;
  desiredFormat.m_dataFormat    = AE_FMT_FLOAT;

  /* if the sink is already open and it is compatible we dont need to do anything */
  if (m_sink && m_sink->IsCompatible(desiredFormat, device))
    return true;

  /* let the thread know we have re-opened the sink */
  m_reOpened = true;

  /* we are going to open, so close the old sink if it was open */
  bool reopen = false;
  if (m_sink)
  {
    m_sink->Deinitialize();
    m_sinkThread->StopThread(true);

    delete m_sink;
    delete m_sinkThread;
    m_sink       = NULL;
    m_sinkThread = NULL;

    _aligned_free(m_buffer);
    m_buffer = NULL;
    reopen = true;
  }

  /* set the local members */
  m_stdChLayout  = stdChLayout;
  m_chLayout     = CAEUtil::GetStdChLayout(stdChLayout);
  m_channelCount = CAEUtil::GetChLayoutCount(m_chLayout);
  CLog::Log(LOGDEBUG, "CAE::Initialize - Using speaker layout: %s", CAEUtil::GetStdChLayoutName(stdChLayout));

  /* create the new sink */
  m_sink = new CAESinkALSA();
  if (!m_sink->Initialize(desiredFormat, device))
  {
    delete m_sink;
    m_sink = NULL;
    return false;
  }

  CLog::Log(LOGINFO, "CAE::Initialize - %s Initialized:", m_sink->GetName());
  CLog::Log(LOGINFO, "  Output Device : %s", device.c_str());
  CLog::Log(LOGINFO, "  Sample Rate   : %d", desiredFormat.m_sampleRate);
  CLog::Log(LOGINFO, "  Channel Count : %d", desiredFormat.m_channelCount);
  CLog::Log(LOGINFO, "  Channel Layout: %s", CAEUtil::GetChLayoutStr(desiredFormat.m_channelLayout).c_str());
  CLog::Log(LOGINFO, "  Frames        : %d", desiredFormat.m_frames);
  CLog::Log(LOGINFO, "  Frame Samples : %d", desiredFormat.m_frameSamples);
  CLog::Log(LOGINFO, "  Frame Size    : %d", desiredFormat.m_frameSize);

  /* get the sink's audio format details as it may have changed them according to what it supports */
  m_format        = desiredFormat;
  m_frameSize     = sizeof(float) * m_channelCount;
  m_convertFn     = CAEConvert::FrFloat(m_format.m_dataFormat);
  m_buffer        = (uint8_t*)_aligned_malloc(m_format.m_frameSize * m_format.m_frames * 2, 16);
  m_bufferFrames  = 0;
  m_visBufferSize = 0;

  /* initialize the final stage remapper */
  m_remap.Initialize(m_chLayout, m_format.m_channelLayout, true);

  /* start the sink thread */
  m_sinkThread = new CThread(m_sink);
  m_sinkThread->Create();

  /* if we did not re-open, we are finished */
  if (!reopen) return true;

  /* re-init sounds */
  m_playing_sounds.clear();
  map<const CStdString, CAESound*>::iterator sitt;
  for(sitt = m_sounds.begin(); sitt != m_sounds.end(); ++sitt)
    sitt->second->Initialize();

  /* re-init streams */
  list<CAEStream*>::iterator itt;
  for(itt = m_streams.begin(); itt != m_streams.end(); ++itt)
    (*itt)->Initialize();

  /* re-init the callback */
  if (m_audioCallback)
  {
    m_audioCallback->OnDeinitialize();
    m_audioCallback->OnInitialize(m_channelCount, m_format.m_sampleRate, 32);
  }

  return true;
}

bool CAE::Initialize()
{
  /* get the current volume level */
  m_volume = g_settings.m_fVolumeLevel;
  return OpenSink();
}

void CAE::Deinitialize()
{
  Stop();

  if (m_sink)
  {
    /* shutdown the sink */
    m_sink->Deinitialize();
    m_sinkThread->StopThread(true);

    delete m_sink;
    delete m_sinkThread;
    m_sink       = NULL;
    m_sinkThread = NULL;
  }

  _aligned_free(m_buffer);
  m_buffer = NULL;
}

void CAE::Stop()
{
  m_running = false;

  /* wait for the thread to stop */
  CSingleLock lock(m_runLock);
}

CAEStream *CAE::GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, bool freeOnDrain/* = false */, bool ownsPostProc/* = false */)
{
  CLog::Log(LOGINFO, "CAE::GetStream - %d, %u, %u, %s",
    CAEUtil::DataFormatToBits(dataFormat),
    sampleRate,
    channelCount,
    CAEUtil::GetChLayoutStr(channelLayout).c_str()
  );

  CSingleLock lock(m_critSection);
  CAEStream *stream = new CAEStream(dataFormat, sampleRate, channelCount, channelLayout, freeOnDrain, ownsPostProc);
  m_streams.push_back(stream);

  /* if we are the only stream, try to re-open the sink to what we want */
  if (m_streams.size() == 1)
    OpenSink();

  return stream;
}

CAESound *CAE::GetSound(CStdString file)
{
  CSingleLock lock(m_critSection);
  CAESound *sound;

  /* see if we have a valid sound */
  if ((sound = m_sounds[file]))
  {
    /* increment the reference count */
    ++sound->m_refcount;
    return sound;
  }

  sound = new CAESound(file);
  if (!sound->Initialize())
  {
    delete sound;
    return NULL;
  }

  m_sounds[file] = sound;
  sound->m_refcount = 1;

  return sound;
}

void CAE::PlaySound(CAESound *sound)
{
   SoundState ss = {
      owner  : sound,
      samples: sound->GetSamples(),
      frames : sound->GetFrameCount()
   };
   CSingleLock lock(m_critSection);
   OpenSink();
   m_playing_sounds.push_back(ss);
}

void CAE::FreeSound(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  /* decrement the sound's ref count */
  --sound->m_refcount;
  ASSERT(sound->m_refcount >= 0);

  /* if other processes are using the sound, dont remove it */
  if (sound->m_refcount > 0)
    return;

  /* set the timeout to 30 seconds */
  sound->m_ts = CTimeUtils::GetTimeMS() + 30000;

  /* stop the sound playing */
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound) itt = m_playing_sounds.erase(itt);
    else ++itt;
  }
}

void CAE::GarbageCollect()
{
  CSingleLock lock(m_critSection);

  unsigned int ts = CTimeUtils::GetTimeMS();
  map<const CStdString, CAESound*>::iterator itt;
  list<map<const CStdString, CAESound*>::iterator> remove;

  for(itt = m_sounds.begin(); itt != m_sounds.end(); ++itt)
  {
    CAESound *sound = itt->second;
    /* free any sounds that are no longer used and are > 30 seconds old */
    if (sound->m_refcount == 0 && ts > sound->m_ts)
    {
      delete sound;
      remove.push_back(itt);
      continue;
    }
  }

  /* erase the entries from the map */
  while(!remove.empty())
  {
    m_sounds.erase(remove.front());
    remove.pop_front();
  }
}

void CAE::RegisterAudioCallback(IAudioCallback* pCallback)
{
  CSingleLock lock(m_critSection);
  m_audioCallback = pCallback;
  if (m_audioCallback)
    m_audioCallback->OnInitialize(m_channelCount, m_format.m_sampleRate, 32);
}

void CAE::UnRegisterAudioCallback()
{
  CSingleLock lock(m_critSection);
  m_audioCallback = NULL;
  m_visBufferSize = 0;
}

void CAE::StopSound(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    if ((*itt).owner == sound) itt = m_playing_sounds.erase(itt);
    else ++itt;
  }
}

bool CAE::IsPlaying(CAESound *sound)
{
  CSingleLock lock(m_critSection);
  list<SoundState>::iterator itt;
  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); ++itt)
    if ((*itt).owner == sound) return true;
  return false;
}

void CAE::RemoveStream(CAEStream *stream)
{
  CSingleLock lock(m_critSection);
  m_streams.remove(stream);
  if (stream->GetDataFormat() == AE_FMT_IEC958)
  {
    m_passthrough = false;
    OpenSink();
  }
}

float CAE::GetDelay()
{
  CSingleLock lock(m_critSection);
  if (!m_running) return 0.0f;

  CSingleLock sinkLock(m_critSectionSink);
  return m_sink->GetDelay() + ((float)(m_bufferFrames / m_frameSize) / m_format.m_sampleRate);
}

float CAE::GetVolume()
{
  CSingleLock lock(m_critSection);
  return m_volume;
}

void CAE::SetVolume(float volume)
{
  CSingleLock lock(m_critSection);
  g_settings.m_fVolumeLevel = volume;
  m_volume = volume;
}

#ifdef __SSE__
inline void CAE::SSEMulAddArray(float *data, float *add, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while((((uintptr_t)data & 0xF) || ((uintptr_t)add & 0xF)) && count > 0)
  {
    data[0] += add[0] * mul;
    ++add;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i+=4, data+=4, add+=4)
  {
    __m128 ad      = _mm_load_ps(add );
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_add_ps (to, _mm_mul_ps(ad, m));
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] += add[0] * mul;
    else
    {
      __m128 ad;
      __m128 to;
      if (odd == 2)
      {
        ad = _mm_setr_ps(add [0], add [1], 0, 0);
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        ad = _mm_setr_ps(add [0], add [1], add [2], 0);
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_add_ps(to, _mm_mul_ps(ad, m));
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}

inline void CAE::SSEMulArray(float *data, const float mul, uint32_t count)
{
  const __m128 m = _mm_set_ps1(mul);

  /* work around invalid alignment */
  while(((uintptr_t)data & 0xF) && count > 0)
  {
    data[0] *= mul;
    ++data;
    --count;
  }

  uint32_t even = count & ~0x3;
  for(uint32_t i = 0; i < even; i+=4, data+=4)
  {
    __m128 to      = _mm_load_ps(data);
    *(__m128*)data = _mm_mul_ps (to, m);
  }

  if (even != count)
  {
    uint32_t odd = count - even;
    if (odd == 1)
      data[0] *= mul;
    else
    {     
      __m128 to;
      if (odd == 2)
      {
        to = _mm_setr_ps(data[0], data[1], 0, 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
      }
      else
      {
        to = _mm_setr_ps(data[0], data[1], data[2], 0);
        __m128 ou = _mm_mul_ps(to, m);
        data[0] = ((float*)&ou)[0];
        data[1] = ((float*)&ou)[1];
        data[2] = ((float*)&ou)[2];
      }
    }
  }
}
#endif

void CAE::Run()
{
  /* we release this when we exit the thread unblocking anyone waiting on "Stop" */
  CSingleLock runLock(m_runLock);
  m_running = true;

  CLog::Log(LOGINFO, "CAE::Run - Thread Started");
  while(m_running)
  {
    unsigned int channelCount, mixed;

    CSingleLock sinkLock(m_critSectionSink);
      m_reOpened = false;
      RunOutputStage();
      /* copy this value so we can unlock the sink */
      channelCount = m_channelCount;
    sinkLock.Leave();

    CSingleLock mixLock(m_critSection);
      DECLARE_ALIGNED(16, float, out[channelCount]);
      memset(out, 0, sizeof(out));

      mixed =
        RunSoundStage (channelCount, out) +
        RunStreamStage(channelCount, out);

      RunNormalizeStage(channelCount, out, mixed);
      RunVizStage      (channelCount, out);
      RunDeAmpStage    (channelCount, out);
    mixLock.Leave();

    sinkLock.Enter();
    if (!m_reOpened)
      RunBufferStage(out);
    sinkLock.Leave();
  }
}

inline void CAE::RunOutputStage()
{
  /* this normally only loops once */
  while(m_bufferFrames >= m_format.m_frames)
  {
    int wroteFrames = m_sink->AddPackets(m_buffer, m_format.m_frames);

    int wroteBytes  = wroteFrames * m_format.m_frameSize;
    int bytesLeft   = (m_bufferFrames - wroteFrames) * m_format.m_frameSize;
    memmove(&m_buffer[0], &m_buffer[wroteBytes], bytesLeft);
    m_bufferFrames -= wroteFrames;
  }
}

inline unsigned int CAE::RunSoundStage(unsigned int channelCount, float *out)
{
  unsigned int mixed = 0;
  list<SoundState>::iterator itt;

  for(itt = m_playing_sounds.begin(); itt != m_playing_sounds.end(); )
  {
    SoundState *ss = &(*itt);

    /* no more frames, so remove it from the list */
    if (ss->frames == 0)
    {
      itt = m_playing_sounds.erase(itt);
      continue;
    }

    float *frame = ss->samples;
    float volume = ss->owner->GetVolume();
    ss->samples += channelCount;
    --ss->frames;

    #ifdef __SSE__
    if (channelCount > 1)
      CAE::SSEMulAddArray(out, frame, volume, channelCount);
    else
    #endif
    for(unsigned int i = 0; i < channelCount; ++i)
      out[i] += frame[i] * volume;

    ++mixed;
    ++itt;
  }

  return mixed;
}

inline unsigned int CAE::RunStreamStage(unsigned int channelCount, float *out)
{
  unsigned int mixed = 0;
  list<CAEStream*>::iterator itt;

  /* mix in any running streams */
  for(itt = m_streams.begin(); itt != m_streams.end();)
  {
    CAEStream *stream = *itt;

    /* delete streams that are flagged for deletion */
    if (stream->m_delete)
    {
      itt = m_streams.erase(itt);
      delete stream;
      continue;
    }

    /* dont process streams that are paused */
    if (stream->IsPaused()) {
      ++itt;
      continue;
    }

    float *frame = stream->GetFrame();
    if (!frame)
    {
      /* if the stream is drained and is set to free on drain */
      if (stream->IsDraining() && stream->IsFreeOnDrain())
      {
        itt = m_streams.erase(itt);
        delete stream;
        continue;
      }

      ++itt;
      continue;
    }

    float volume = stream->GetVolume();
    #ifdef __SSE__
    if (channelCount > 1)
      CAE::SSEMulAddArray(out, frame, volume, channelCount);
    else
    #endif
    {
      for(unsigned int i = 0; i < channelCount; ++i)
        out[i] += frame[i] * volume;
    }

    ++mixed;
    ++itt;
  }

  return mixed;
}

inline void CAE::RunNormalizeStage(unsigned int channelCount, float *out, unsigned int mixed)
{
  if (mixed <= 0) return;

  float mul = 1.0f / mixed;
  #ifdef __SSE__
  if (channelCount > 1)
    CAE::SSEMulArray(out, mul, channelCount);
  else
  #endif
  {
    for(unsigned int i = 0; i < channelCount; ++i)
      out[i] *= mul;
  }
}

inline void CAE::RunVizStage(unsigned int channelCount, float *out)
{
  /* if we have an audio callback, use it */
  if (!m_audioCallback) return;

  /* add the frame to the visBuffer */
  memcpy(&m_visBuffer[m_visBufferSize], out, sizeof(float) * channelCount);
  m_visBufferSize += channelCount;

  /* if the buffer full, flush it through */
  if (m_visBufferSize >= AUDIO_BUFFER_SIZE)
  {
    m_audioCallback->OnAudioData(m_visBuffer, AUDIO_BUFFER_SIZE);
    m_visBufferSize = 0;
  }
}

inline void CAE::RunDeAmpStage(unsigned int channelCount, float *out)
{
  if (g_settings.m_bMute || m_volume == 1.0f) return;

  #ifdef __SSE__
  if (channelCount > 1)
    CAE::SSEMulArray(out, m_volume, channelCount);
  else
  #endif
  {
    for(unsigned int i = 0; i < channelCount; ++i)
      out[i] *= m_volume;
  }
}

inline void CAE::RunBufferStage(float *out)
{
  uint8_t *outPos = &m_buffer[m_bufferFrames * m_format.m_frameSize];

  /* if muted just zero the data and continue */
  if (g_settings.m_bMute)
  {
    memset(outPos, 0, m_format.m_frameSize);
    ++m_bufferFrames;
    return;
  }

  if (m_convertFn)
  {
    DECLARE_ALIGNED(16, float, remapped[m_format.m_channelCount]);
    m_remap.Remap  (out, remapped, 1);
    m_convertFn    (remapped, m_format.m_channelCount, outPos);
  }
  else
    m_remap.Remap(out, (float*)outPos, 1);

  ++m_bufferFrames;
}

