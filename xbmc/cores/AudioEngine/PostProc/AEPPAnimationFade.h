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

#ifndef AEPPANIMATIONFADE_H
#define AEPPANIMATIONFADE_H

#include "AEPostProc.h"

class CAEPPAnimationFade : public IAEPostProc
{
public:
  virtual bool        Initialize(CAEStream *stream);
  virtual void        Drain();
  virtual void        Process(float *data, unsigned int samples);
  virtual const char* GetName() { return "AnimationFade"; }

  void Run();
  void Stop();
  void SetPosition(const float position);
private:
  unsigned int m_sampleRate;   /* the AE sample rate   */
  unsigned int m_channelCount; /* the AE channel count */

  bool  m_running;  /* if the fade is running */
  float m_position; /* current fade position */
  float m_from;     /* fade from */
  float m_to;       /* fade to */
  float m_length;   /* fade time in ms */
  float m_step;     /* the fade step size */
};

#endif
