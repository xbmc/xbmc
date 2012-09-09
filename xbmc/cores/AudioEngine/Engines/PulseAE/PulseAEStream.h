#pragma once
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

#ifdef HAS_PULSEAUDIO

#include "Interfaces/AEStream.h"
#include "threads/Thread.h"
#include <pulse/pulseaudio.h>

class CPulseAEStream : public IAEStream
{
public:
  /* this should NEVER be called directly, use AE.GetStream */
  CPulseAEStream(pa_context *context, pa_threaded_mainloop *mainLoop, enum AEDataFormat format, unsigned int sampleRate, CAEChannelInfo channelLayout, unsigned int options);
  virtual ~CPulseAEStream();

  virtual void Destroy();

  virtual unsigned int GetSpace();
  virtual unsigned int AddData(void *data, unsigned int size);
  virtual double GetDelay();
  virtual double GetCacheTime ();
  virtual double GetCacheTotal();

  virtual bool IsPaused     ();
  virtual bool IsDraining   ();
  virtual bool IsDrained    ();
  virtual bool IsDestroyed  ();
  virtual bool IsBuffering() { return false; }

  virtual void Pause   ();
  virtual void Resume  ();
  virtual void Drain   ();
  virtual void Flush   ();

  virtual float GetVolume    ();
  virtual float GetReplayGain();
  virtual void  SetVolume    (float volume);
  virtual void  SetReplayGain(float factor);
  void SetMute(const bool muted);

  virtual const unsigned int      GetFrameSize   () const;
  virtual const unsigned int      GetChannelCount() const;
  virtual const unsigned int      GetSampleRate  () const;
  virtual const enum AEDataFormat GetDataFormat  () const;
  virtual const unsigned int GetEncodedSampleRate() const { return GetSampleRate(); }

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio();
  virtual bool   SetResampleRatio(double ratio);

  /* vizualization callback register function */
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual void FadeVolume(float from, float target, unsigned int time);
  virtual bool IsFading();

  /* trigger the stream to update its volume relative to AE */
  void UpdateVolume(float max);

  virtual void RegisterSlave(IAEStream *stream);
private:
  static void StreamRequestCallback(pa_stream *s, size_t length, void *userdata);
  static void StreamLatencyUpdateCallback(pa_stream *s, void *userdata);
  static void StreamStateCallback(pa_stream *s, void *userdata);
  static void StreamUnderflowCallback(pa_stream *s, void *userdata);
  static void StreamDrainComplete(pa_stream *s, int success, void *userdata);

  static inline bool WaitForOperation(pa_operation *op, pa_threaded_mainloop *mainloop, const char *LogEntry);
  bool Cork(bool cork);

  bool m_Destroyed;
  bool m_Initialized;
  bool m_Paused;

  pa_stream *m_Stream;
  pa_sample_spec m_SampleSpec;

  float       m_MaxVolume;
  float       m_Volume;
  pa_cvolume  m_ChVolume;

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;

  IAudioCallback* m_AudioCallback;

  enum AEDataFormat m_format;
  unsigned int      m_sampleRate;
  CAEChannelInfo    m_channelLayout;
  unsigned int m_options;
  unsigned int m_frameSize;
  unsigned int m_cacheSize;

  pa_operation *m_DrainOperation;
  IAEStream    *m_slave;

  class CLinearFader : public CThread
  {
  public:
    CLinearFader(IAEStream *stream);
    void SetupFader(float from, float target, unsigned int time);
    bool IsRunning();
  protected:
    virtual void Process();
  private:
    IAEStream *m_stream;
    float m_from;
    float m_target;
    unsigned int m_time;
    volatile bool m_isRunning;
  } m_fader;
};

#endif
