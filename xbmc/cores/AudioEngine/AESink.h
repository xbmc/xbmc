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

#include "utils/Thread.h"
#include "AEAudioFormat.h"
#include "StdString.h"
#include <stdint.h>

class IAESink : public IRunnable
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
  virtual bool Initialize  (AEAudioFormat &format, CStdString &device) = 0;

  /*
    Deinitialize the sink for destruction
  */
  virtual void Deinitialize() = 0;

  /*
    Return true if the supplied format and device are compatible with the current open sink
  */
  virtual bool IsCompatible(const AEAudioFormat format, const CStdString device) = 0;

  /*
    This method must block while the sink is stopping
  */
  virtual void Stop() = 0;

  /*
    This method must return the delay in miliseconds till new data will be sent out
  */
  virtual float GetDelay() = 0;

  /*
    This method must buffer packets to be sent out, the format of data and MUST
    block when the buffer is full. Be very careful about synchronization here.
  */
  virtual unsigned int AddPackets(uint8_t *data, unsigned int samples) = 0;

  /*
    This is the THREAD that sends out the buffered data, and MUST unblock AddPackets
    when there is a free buffer. Be very careful about synchronization here.
  */
  virtual void Run() = 0;
};

