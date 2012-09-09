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

#include "Interfaces/AESound.h"
#include "Utils/AEWAVLoader.h"
#include <pulse/pulseaudio.h>

class CPulseAESound : public IAESound
{
public:
  /* this should NEVER be called directly, use AE.GetSound */
  CPulseAESound(const std::string &filename, pa_context *context, pa_threaded_mainloop *mainLoop);
  virtual ~CPulseAESound();

  virtual void DeInitialize();
  virtual bool Initialize();

  virtual void Play();
  virtual void Stop();
  virtual bool IsPlaying();

  virtual void  SetVolume(float volume);
  virtual float GetVolume();
private:
  static void StreamStateCallback(pa_stream *s, void *userdata);
  static void StreamWriteCallback(pa_stream *s, size_t length, void *userdata);
  void Upload(size_t length);

  std::string    m_pulseName;
  std::string    m_filename;
  CAEWAVLoader  m_wavLoader;
  size_t        m_dataSent;

  pa_context           *m_context;
  pa_threaded_mainloop *m_mainLoop;
  pa_stream            *m_stream;
  pa_sample_spec        m_sampleSpec;
  pa_cvolume            m_chVolume;
  pa_operation         *m_op;

  float m_maxVolume, m_volume;
};

#endif
