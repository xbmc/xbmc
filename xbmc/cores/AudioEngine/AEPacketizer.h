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

#include "AEAudioFormat.h"
#include <stdint.h>

class IAEPacketizer
{
public:
  virtual const char*  GetName() = 0;
  virtual unsigned int GetPacketSize() = 0;

  IAEPacketizer() {};
  virtual ~IAEPacketizer() {};
  virtual bool Initialize() = 0;
  virtual void Deinitialize() = 0;
  virtual void Reset() = 0;

  virtual int  AddData  (uint8_t *data, unsigned int size) = 0;
  virtual bool HasPacket() = 0;
  virtual int  GetPacket(uint8_t **data) = 0;
  virtual void DropPacket() = 0;
  virtual unsigned int GetSampleRate() = 0;
  virtual unsigned int GetBufferSize() = 0;
};

