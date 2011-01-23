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

#ifndef __CHANNELS_H
#define __CHANNELS_H

#include "libXBMC_pvr.h"
#include <string>

class cChannel
{
private:
  std::string name;
  int uid;
  int external_id;
  bool encrypted;

public:
  cChannel(const PVR_CHANNEL *Channel);
  cChannel();
  virtual ~cChannel();

  bool Parse(const std::string& data);
  const char *Name(void) const { return name.c_str(); }
  int UID(void) const { return uid; }
  int ExternalID(void) const { return external_id; }
  bool Encrypted(void) const { return encrypted; }
};

#endif //__TIMERS_H
