#pragma once
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

#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AELimiter.h"

namespace ActiveAE
{

class CActiveAEStream : public IAEStream
{
protected:
  friend class CActiveAE;
  friend class CEngineStats;
  CActiveAEStream(AEAudioFormat *format);
  virtual ~CActiveAEStream();
  void FadingFinished();
  void IncFreeBuffers();
  void DecFreeBuffers();
  void ResetFreeBuffers();
  void InitRemapper();
  void RemapBuffer();

public:
  virtual unsigned int GetSpace();
  virtual unsigned int AddData(uint8_t* const *data, unsigned int offset, unsigned int frames, double pts = 0.0);
  virtual double GetDelay();
  virtual int64_t GetPlayingPTS();
  virtual bool IsBuffering();
  virtual double GetCacheTime();
  virtual double GetCacheTotal();

  virtual void Pause();
  virtual void Resume();
  virtual void Drain(bool wait);
  virtual bool IsDraining();
  virtual bool IsDrained();
  virtual void Flush();

  virtual float GetVolume();
  virtual float GetReplayGain();
  virtual float GetAmplification();
  virtual void SetVolume(float volume);
  virtual void SetReplayGain(float factor);
  virtual void SetAmplification(float amplify);

  virtual const unsigned int GetFrameSize() const;
  virtual const unsigned int GetChannelCount() const;
  
  virtual const unsigned int GetSampleRate() const ;
  virtual const unsigned int GetEncodedSampleRate() const;
  virtual const enum AEDataFormat GetDataFormat() const;
  
  virtual double GetResampleRatio();
  virtual bool SetResampleRatio(double ratio);
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();
  virtual void FadeVolume(float from, float to, unsigned int time);
  virtual bool IsFading();
  virtual void RegisterSlave(IAEStream *stream);
  virtual void Discontinuity();

protected:

  AEAudioFormat m_format;
  float m_streamVolume;
  float m_streamRgain;
  float m_streamAmplify;
  double m_streamResampleRatio;
  unsigned int m_streamSpace;
  bool m_streamDraining;
  bool m_streamDrained;
  bool m_streamFading;
  int m_streamFreeBuffers;
  bool m_streamIsBuffering;
  bool m_streamIsFlushed;
  IAEStream *m_streamSlave;
  CCriticalSection m_streamLock;
  uint8_t *m_leftoverBuffer;
  int m_leftoverBytes;
  CSampleBuffer *m_currentBuffer;
  CSoundPacket *m_remapBuffer;
  IAEResample *m_remapper;
  int m_clockId;

  // only accessed by engine
  CActiveAEBufferPool *m_inputBuffers;
  CActiveAEBufferPoolResample *m_resampleBuffers;
  std::deque<CSampleBuffer*> m_processingSamples;
  CActiveAEDataProtocol *m_streamPort;
  CEvent m_inMsgEvent;
  CCriticalSection *m_statsLock;
  bool m_drain;
  bool m_paused;
  bool m_started;
  CAELimiter m_limiter;
  float m_volume;
  float m_rgain;
  float m_amplify;
  float m_bufferedTime;
  int m_fadingSamples;
  float m_fadingBase;
  float m_fadingTarget;
  int m_fadingTime;
  bool m_forceResampler;
};
}

