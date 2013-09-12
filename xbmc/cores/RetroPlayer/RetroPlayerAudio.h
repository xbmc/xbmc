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
#pragma once

#include "RetroPlayerBuffer.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <stdint.h>
#include <vector>

#define AUDIO_BUFFER_LENGTH_MS  70 // Buffer up to 70ms of audio (~4 frames @ 60fps)

class IAEStream;

class CRetroPlayerAudio : public CThread
{
  struct AudioInfo { };

  class CRetroPlayerAudioBuffer : public CRetroPlayerBuffer
  {
  public:
    CRetroPlayerAudioBuffer(CRetroPlayerAudio *audio) : m_audio(audio) { }
    virtual ~CRetroPlayerAudioBuffer() { }

  protected:
    virtual bool IsFull() const
    {
      const unsigned int NUM_CHANNELS = 2;
      const unsigned int SIZE_FRAME = NUM_CHANNELS * sizeof(uint16_t);
      const double buffertime = (m_audio && m_audio->GetSampleRate()) ?
        1000.0 * GetSize() / SIZE_FRAME / m_audio->GetSampleRate() : 0.0; // ms
      return buffertime > AUDIO_BUFFER_LENGTH_MS;
    }

  private:
    CRetroPlayerAudio *m_audio;
  };

  // No audio metadata, so just use CRetroPlayerPacketBase
  typedef CRetroPlayerPacket<AudioInfo> AudioPacket;

public:
  CRetroPlayerAudio();
  ~CRetroPlayerAudio();

  /**
   * Rev up the engines and start the thread.
   * @param  samplerate - the desired samplerate
   * @return the chosen samplerate, or 0 if failure
   */
  unsigned int GoForth(double samplerate);

  /**
   * Send audio samples to be processed by this class. Data format is:
   * int16_t buf[4] = { l, r, l, r }; this would be 2 frames.
   */
  void SendAudioFrames(const int16_t *data, size_t frames);

  /**
   * Send a single frame. Frames are cached until Flush() is called.
   */
  void SendAudioFrame(int16_t left, int16_t right);

  /**
   * Send the buffered audio to the audio engine if using single-frame audio.
   */
  void Flush();

  /**
   * Accumulative audio delay. Does not include delay due to current packet, so
   * at 60fps this could be up to 17ms (~1/60) behind. Accuracy is also subject
   * to accuracy found in AE GetDelay() functions.
   */
  double GetDelay() const;

  unsigned int GetSampleRate() const;

protected:
  virtual void Process();

private:
  // libretro cores can send audio samples in a batch, or frame-by-frame
  enum AUDIO_FRAME_TYPE
  {
    FRAME_TYPE_UNKNOWN, // Initial state
    FRAME_TYPE_SINGLE,  // Audio arrives via SendAudioFrame()
    FRAME_TYPE_SAMPLES  // Audio arrives via SendAudioFrames()
  };

  void ProcessPacket(const AudioPacket &packet);

  /**
   * Given a desired samplerate, this will choose an appropriate sample rate
   * depending on the user's hardware.
   * @param  samplerate - the desired samplerate
   * @return the chosen samplerate
   */
  static unsigned int FindSampleRate(double samplerate);

  IAEStream               *m_pAudioStream;
  CRetroPlayerAudioBuffer m_buffer;
  AUDIO_FRAME_TYPE        m_frameType; // Set when SendAudioFrame[s]() is first called
  CEvent                  m_packetReady;
  std::vector<int16_t>    m_singleFrameBuffer; // used if m_frameType == FRAME_TYPE_SINGLE
};
