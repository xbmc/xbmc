#pragma once
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

#include "AEStream.h"
#include "threads/SharedSection.h"
#include <list>

class IAEStream;

class CAEStreamWrapper : public IAEStream
{
protected:
  friend class CAEWrapper;

  CAEStreamWrapper(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options);
  virtual ~CAEStreamWrapper();

  void AlterStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options);
  void FreeStream();

  /* used by the wrapper to signal fetching a new stream */
  void UnInitialize();
  void Initialize  ();
public:
  virtual void Destroy();
  virtual void DisableCallbacks(bool free = true);
  virtual void SetDataCallback (AECBFunc *cbFunc, void *arg);
  virtual void SetDrainCallback(AECBFunc *cbFunc, void *arg);
  virtual void SetFreeCallback (AECBFunc *cbFunc, void *arg);
  virtual unsigned int AddData(void *data, unsigned int size);
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();
  virtual void Pause();
  virtual void Resume();
  virtual void Drain();
  virtual bool IsDraining();
  virtual void Flush();
  virtual float GetVolume();
  virtual void  SetVolume(float volume);
  virtual float GetReplayGain();
  virtual void  SetReplayGain(float factor);
  virtual void AppendPostProc (IAEPostProc *pp);
  virtual void PrependPostProc(IAEPostProc *pp);
  virtual void RemovePostProc (IAEPostProc *pp);
  virtual unsigned int GetFrameSize();
  virtual unsigned int GetChannelCount();
  virtual unsigned int GetSampleRate();
  virtual enum AEDataFormat GetDataFormat();
  virtual double GetResampleRatio();
  virtual void   SetResampleRatio(double ratio);
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

private:
  IAE               *m_ae;
  enum AEDataFormat m_dataFormat;
  unsigned int      m_sampleRate;
  unsigned int      m_channelCount;
  AEChLayout        m_channelLayout;
  unsigned int      m_options;

  CSharedSection          m_lock;
  IAEStream              *m_stream;
  float                   m_volume;
  float                   m_replayGain;
  double                  m_resampleRatio;
  double                  m_streamRatio;
  IAudioCallback         *m_callback;
  std::list<IAEPostProc*> m_postproc;
  AECBFunc               *m_dataCallback;
  void                   *m_dataCallbackArg;
  AECBFunc               *m_drainCallback;
  void                   *m_drainCallbackArg;
  AECBFunc               *m_freeCallback;
  void                   *m_freeCallbackArg;

  static void StaticStreamOnData (IAEStream *sender, void *arg, unsigned int needed);
  static void StaticStreamOnDrain(IAEStream *sender, void *arg, unsigned int unused);
  static void StaticStreamOnFree (IAEStream *sender, void *arg, unsigned int unused);
};

