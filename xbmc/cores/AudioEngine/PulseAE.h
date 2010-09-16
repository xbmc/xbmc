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

#include "AE.h"

struct pa_context;
struct pa_threaded_mainloop;
struct pa_stream;

class CPulseAE : public IAE
{
public:
  /* this should NEVER be called directly, use CAEFactory */
  CPulseAE();
  virtual ~CPulseAE();

  virtual bool  Initialize      ();
  virtual void  OnSettingsChange(CStdString setting);

  virtual float GetVolume();
  virtual void  SetVolume(float volume);

  /* returns a new stream for data in the specified format */
  virtual IAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0);

  /* returns a new sound object */
  virtual IAESound *GetSound(CStdString file);
  virtual void FreeSound(IAESound *sound);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
private:
  static void ContextStateCallback(pa_context *c, void *userdata);

  pa_context *m_Context;
  pa_threaded_mainloop *m_MainLoop;
};
