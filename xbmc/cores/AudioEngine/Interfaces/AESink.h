#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/Thread.h"
#include "AE.h"
#include "AEAudioFormat.h"
#include "utils/StdString.h"
#include <stdint.h>

class IAESink
{
public:
  /* return the name of this sync for logging */
  virtual const char *GetName() = 0;

  IAESink() {};
  virtual ~IAESink() {};

  /*
    The sink does NOT have to honour anything in the format struct or the device
    if however it does not honour what is requested, it MUST update device/format
    with what it does support.
  */
  virtual bool Initialize  (AEAudioFormat &format, std::string &device) = 0;

  /*
    Deinitialize the sink for destruction
  */
  virtual void Deinitialize() = 0;

  /*
    Return true if the supplied format and device are compatible with the current open sink
  */
  virtual bool IsCompatible(const AEAudioFormat format, const std::string device) = 0;

  /*
    This method returns the time in seconds that it will take
    for the next added packet to be heard from the speakers.
  */
  virtual double GetDelay() = 0;

  /*
    This method returns the time in seconds that it will take
    to underrun the cache if no sample is added.
  */
  virtual double GetCacheTime() = 0;

  /*
    This method returns the total time in seconds of the cache.
  */
  virtual double GetCacheTotal() = 0;

  /*
    Adds packets to be sent out, this routine MUST block or sleep.
  */
  virtual unsigned int AddPackets(uint8_t *data, unsigned int frames, bool hasAudio) = 0;

  /*
    Drain the sink
   */
  virtual void Drain() {};

  /*
    Indicates if sink can handle volume control.
  */
  virtual bool  HasVolume() {return false;};

  /*
    This method sets the volume control, volume ranges from 0.0 to 1.0.
  */
  virtual void  SetVolume(float volume) {};
};

