/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "system.h"
#include "RetroPlayerAudio.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEConvert.h"
#include "cores/dvdplayer/DVDClock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

// Temporary - notify users about unsupported samplerate
#include "dialogs/GUIDialogOK.h"

#ifdef TARGET_WINDOWS // disable C4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable: 4355)
#endif

// Pre-convert audio to float to avoid additional buffer in AE stream
#define CHANNELS      2 // L + R
#define FRAMESIZE     (CHANNELS * sizeof(float))

using namespace std;

CRetroPlayerAudio::CRetroPlayerAudio()
  : CThread("RetroPlayerAudio"), 
    m_pAudioStream(NULL),
    m_buffer(this),
    m_frameType(FRAME_TYPE_UNKNOWN)
{
}

CRetroPlayerAudio::~CRetroPlayerAudio()
{
  StopThread();
}

unsigned int CRetroPlayerAudio::GoForth(double samplerate)
{
  if (m_pAudioStream)
    { CAEFactory::FreeStream(m_pAudioStream); m_pAudioStream = NULL; }

  const unsigned int newsamplerate = FindSampleRate(samplerate);

  CLog::Log(LOGINFO, "RetroPlayerAudio: Creating audio stream, sample rate hint = %u", newsamplerate);
  static enum AEChannel map[3] = {AE_CH_FL, AE_CH_FR, AE_CH_NULL};
  m_pAudioStream = CAEFactory::MakeStream(AE_FMT_FLOAT, newsamplerate, newsamplerate, CAEChannelInfo(map), AESTREAM_AUTOSTART);

  if (!m_pAudioStream)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Failed to create audio stream");
    return 0;
  }
  else if (newsamplerate != m_pAudioStream->GetSampleRate())
  {
    // For real-time (ish) audio, we need to avoid resampling
    CLog::Log(LOGERROR, "RetroPlayerAudio: sink sample rate (%u) doesn't match", m_pAudioStream->GetSampleRate());
    // Temporary: Notify the user via GUI box
    string msg = StringUtils::Format("Sample rate not supported by audio device: %uHz", samplerate);
    CGUIDialogOK::ShowAndGetInput(257, msg.c_str(), "Continuing without sound", 0);
    return 0;
  }

  Create();
  return newsamplerate;
}

/* static */
unsigned int CRetroPlayerAudio::FindSampleRate(double samplerate)
{
  // List comes from AESinkALSA.cpp
  static unsigned int sampleRateList[] = {5512, 8000, 11025, 16000, 22050, 32000, 44100, 48000, 0};
  for (unsigned int *rate = sampleRateList; ; rate++)
  {
    if (*(rate + 1) == 0)
    {
      // Reached the end of our list
      return *rate;
    }
    if ((unsigned int)samplerate < (*rate + *(rate + 1)) / 2)
    {
      // samplerate is between this rate and the next, so use this rate
      return *rate;
    }
  }
}

void CRetroPlayerAudio::Process()
{
  if (!m_pAudioStream)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Process() called with no audio stream!");
    return;
  }

  while (!m_bStop)
  {
    // RetroPlayerAudio is greedy (consumes all available frames)
    AudioPacket *packet = NULL;
    m_buffer.GetPacket(packet);
    if (!packet)
    {
      if (AbortableWait(m_packetReady, 17) == WAIT_INTERRUPTED) // ~1 video frame @ 60fps
        break;
    }

    if (packet)
      ProcessPacket(*packet);
  }

  // Clean up
  CAEFactory::FreeStream(m_pAudioStream);
  m_pAudioStream = NULL;
}

void CRetroPlayerAudio::ProcessPacket(const AudioPacket &packet)
{
  // Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay() + packet.buffer.size() / FRAMESIZE / (double)m_pAudioStream->GetSampleRate());
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += CDVDClock::GetAbsoluteClock();

  // Keep track of how much data has been added to the stream
  const uint8_t *dataPtr = packet.buffer.data();
  unsigned int size = packet.buffer.size();
  do
  {
    // Fast-forward packet data on successful add
    unsigned int copied = m_pAudioStream->AddData(const_cast<uint8_t*>(dataPtr), size);
    dataPtr += copied;
    size -= copied;

    // Test for incomplete frames remaining
    if (size < FRAMESIZE)
      break;

    if (copied == 0 && timeout < CDVDClock::GetAbsoluteClock())
    {
      CLog::Log(LOGERROR, "RetroPlayerAudio: Timeout adding data to audio renderer");
      break;
    }

    Sleep(1);
  } while (!m_bStop);

  // Discard extra data
  if (size > 0 && !m_bStop)
    CLog::Log(LOGNOTICE, "RetroPlayerAudio: %u bytes left over after rendering, discarding", size);
}

double CRetroPlayerAudio::GetDelay() const
{
  return !m_bStop && m_pAudioStream ? m_pAudioStream->GetDelay() : 0.0;
}

unsigned int CRetroPlayerAudio::GetSampleRate() const
{
  return !m_bStop && m_pAudioStream ? m_pAudioStream->GetSampleRate() : 0;
}

void CRetroPlayerAudio::SendAudioFrames(const int16_t *data, size_t frames)
{
  if (m_frameType == FRAME_TYPE_UNKNOWN)
  {
    CLog::Log(LOGNOTICE, "RetroPlayerAudio: Using multi-sample audio frames");
    m_frameType = FRAME_TYPE_SAMPLES;
  }

  const unsigned int SAMPLES = frames * CHANNELS;

  // Use a static buffer for float conversion
  static vector<float> dest;
  if (dest.size() < SAMPLES)
    dest.resize(SAMPLES);

  // Batch-convert our audio samples to floats here using AE converters. We
  // do this to avoid conversion in AE, which reqiures another buffer and
  // another source of delay.
  uint8_t *source = reinterpret_cast<uint8_t*>(const_cast<int16_t*>(data));
  static CAEConvert::AEConvertToFn convertFn = CAEConvert::ToFloat(AE_FMT_S16NE);
  convertFn(source, SAMPLES, dest.data());

  AudioInfo info = { };

  m_buffer.AddPacket(reinterpret_cast<const uint8_t*>(dest.data()), SAMPLES * sizeof(float), info);

  m_packetReady.Set();
}

void CRetroPlayerAudio::SendAudioFrame(int16_t left, int16_t right)
{
  if (m_frameType == FRAME_TYPE_UNKNOWN)
  {
    CLog::Log(LOGNOTICE, "RetroPlayerAudio: Using single-frame audio");
    m_frameType = FRAME_TYPE_SINGLE;
  }

  m_singleFrameBuffer.push_back(left);
  m_singleFrameBuffer.push_back(right);
}

void CRetroPlayerAudio::Flush()
{
  if (!m_singleFrameBuffer.empty())
    SendAudioFrames(m_singleFrameBuffer.data(), m_singleFrameBuffer.size() / CHANNELS);

  m_singleFrameBuffer.clear();
}

#ifdef TARGET_WINDOWS
#pragma warning(pop)
#endif
