#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
    This method must return the delay in seconds till new data will be sent out
  */
  virtual double GetDelay() = 0;

  /*
    This method returns the time in seconds till the sink's cache is full
  */
  virtual double GetCacheTime() = 0;

  /*
    This method returns the total length of the cache in seconds
  */
  virtual double GetCacheTotal() = 0;

  /*
    Adds packets to be sent out, must block after at-least one block is being rendered
  */
  virtual unsigned int AddPackets(uint8_t *data, unsigned int frames) = 0;

  /*
    Drain the sink
   */
  virtual void Drain() {};
};

