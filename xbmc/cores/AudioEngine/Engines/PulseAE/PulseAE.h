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

#include "system.h"
#ifdef HAS_PULSEAUDIO

#include "Interfaces/AE.h"
#include "PulseAEStream.h"
#include "PulseAESound.h"
#include "threads/CriticalSection.h"
#include <list>

struct pa_context;
struct pa_threaded_mainloop;
struct pa_stream;

class CPulseAEStream;
class CPulseAESound;
class CPulseAE : public IAE
{
protected:
  friend class CAEFactory;
  CPulseAE();
  virtual ~CPulseAE();

public:
  virtual bool  CanInit();
  virtual bool  Initialize      ();
  virtual void  OnSettingsChange(const std::string& setting);

  virtual bool  Suspend(); /* Suspend output and de-initialize exclusive sink for external players and power savings */

  virtual bool  IsSuspended();
  virtual bool  Resume();  /* Resume ouput and re-initialize sink after Suspend() above */

  virtual float GetVolume();
  virtual void  SetVolume(float volume);

  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);
  void RemoveStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  virtual void      FreeSound(IAESound *sound);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  virtual void SetMute(const bool enabled);
  virtual bool IsMuted() { return m_muted; }
  virtual void SetSoundMode(const int mode) {}
#if PA_CHECK_VERSION(1,0,0)
  virtual bool SupportsRaw() { return true; }
#endif

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
private:
  CCriticalSection m_lock;
  std::list<CPulseAEStream*> m_streams;
  std::list<CPulseAESound* > m_sounds;

  static void ContextStateCallback(pa_context *c, void *userdata);

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;
  float m_Volume;
  bool m_muted;
};

#endif
