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

#include "AudioEngine/AEStream.h"
#include "AudioEngine/AEPostProc.h"
#include "IAudioCallback.h"

class CAEPPAnimationFade : public IAEPostProc
{
public:
  CAEPPAnimationFade(float from, float to, unsigned int duration);
  ~CAEPPAnimationFade();

  virtual bool        Initialize(IAEStream *stream);
  virtual void        DeInitialize();
  virtual void        Flush();
  virtual void        Process(float *data, unsigned int frames);
  virtual const char* GetName() { return "AnimationFade"; }

  void Run();
  void Stop();
  void SetPosition(const float position);
  void SetDuration(const unsigned int duration);

  typedef void (DoneCallback)(CAEPPAnimationFade *sender, void *arg);
  void SetDoneCallback(DoneCallback *callback, void *arg) { m_callback = callback; m_cbArg = arg; }
private:
  unsigned int  m_channelCount; /* the AE channel count */
  IAEStream    *m_stream;

  bool          m_running;  /* if the fade is running */
  float         m_position; /* current fade position */
  float         m_from;     /* fade from */
  float         m_to;       /* fade to */
  unsigned int  m_duration; /* fade duration in ms */
  float         m_step;     /* the fade step size */
  DoneCallback *m_callback; /* callback for on completion of fade */
  void         *m_cbArg;    /* the argument to pass to the callback function */
};

