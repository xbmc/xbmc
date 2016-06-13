/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include <stdint.h>
#include <limits.h>

#include "AESinkNULL.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

CAESinkNULL::CAESinkNULL()
  : CThread("AESinkNull"),
    m_draining(false),
    m_sink_frameSize(0),
    m_sinkbuffer_size(0),
    m_sinkbuffer_level(0),
    m_sinkbuffer_sec_per_byte(0)
{
}

CAESinkNULL::~CAESinkNULL()
{
}

bool CAESinkNULL::Initialize(AEAudioFormat &format, std::string &device)
{
  // setup for a 250ms sink feed from SoftAE 
  format.m_dataFormat    = (format.m_dataFormat == AE_FMT_RAW) ? AE_FMT_S16NE : AE_FMT_FLOAT;
  format.m_frames        = format.m_sampleRate / 1000 * 250;
  format.m_frameSize     = format.m_channelLayout.Count() * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);
  m_format = format;

  // setup a pretend 500ms internal buffer
  m_sink_frameSize = format.m_channelLayout.Count() * CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3;
  m_sinkbuffer_size = m_sink_frameSize * format.m_sampleRate / 2;
  m_sinkbuffer_sec_per_byte = 1.0 / (double)(m_sink_frameSize * format.m_sampleRate);

  m_draining = false;
  m_wake.Reset();
  m_inited.Reset();
  Create();
  if (!m_inited.WaitMSec(100))
  {
    while(!m_inited.WaitMSec(1))
      Sleep(10);
  }

  return true;
}

void CAESinkNULL::Deinitialize()
{
  // force m_bStop and set m_wake, if might be sleeping.
  m_bStop = true;
  StopThread();
}

void CAESinkNULL::GetDelay(AEDelayStatus& status)
{
  double sinkbuffer_seconds_to_empty = m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer_level;
  status.SetDelay(sinkbuffer_seconds_to_empty);
}

double CAESinkNULL::GetCacheTotal()
{
  return m_sinkbuffer_sec_per_byte * (double)m_sinkbuffer_size;
}

unsigned int CAESinkNULL::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  unsigned int max_frames = (m_sinkbuffer_size - m_sinkbuffer_level) / m_sink_frameSize;
  if (frames > max_frames)
    frames = max_frames;

  if (frames)
  {
    m_sinkbuffer_level += frames * m_sink_frameSize;
    m_wake.Set();
  }

  return frames;
}

void CAESinkNULL::Drain()
{
  m_draining = true;
  m_wake.Set();
}

void CAESinkNULL::EnumerateDevices (AEDeviceList &devices, bool passthrough)
{
  // we never return any devices
}

void CAESinkNULL::Process()
{
  CLog::Log(LOGDEBUG, "CAESinkNULL::Process");

  // The object has been created and waiting to play,
  m_inited.Set();
  // yield to give other threads a chance to do some work.
  Sleep(0);

  SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
  while (!m_bStop)
  {
    if (m_draining)
    {
      //! @todo is it correct to not take data at the appropriate rate while draining?
      m_sinkbuffer_level = 0;
      m_draining = false;
    }

    // pretend we have a 64k audio buffer
    unsigned int min_buffer_size = 64 * 1024;
    unsigned int read_bytes = m_sinkbuffer_level;
    if (read_bytes > min_buffer_size)
      read_bytes = min_buffer_size;

    if (read_bytes > 0)
    {
      // drain it
      m_sinkbuffer_level -= read_bytes;

      // we MUST drain at the correct audio sample rate
      // or the NULL sink will not work right. So calc
      // an approximate sleep time.
      int frames_written = read_bytes / m_sink_frameSize;
      double empty_ms = 1000.0 * (double)frames_written / m_format.m_sampleRate;
      #if defined(TARGET_POSIX)
        usleep(empty_ms * 1000.0);
      #else
        Sleep((int)empty_ms);
      #endif
    }

    if (m_sinkbuffer_level == 0)
    {
      // sleep this audio thread, we will get woken when we have audio data.
      m_wake.WaitMSec(250);
    }
  }
  SetPriority(THREAD_PRIORITY_NORMAL);
}
