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

#ifndef __TIMERS_H
#define __TIMERS_H

#include "libXBMC_pvr.h"
#include <stdlib.h>
#include <string>

/*
enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };
*/
class cTimer
{
private:
  time_t m_starttime, m_stoptime;
  int m_priority;
  int m_channel;
  string m_title;
  string m_directory;
  int m_index;
  time_t m_UTCdiff;

public:
  cTimer();
  virtual ~cTimer();

  int Index(void) const { return m_index; }
  unsigned int Channel(void) const { return m_channel; }
  int Priority(void) const { return m_priority; }
  const char* Title(void) const { return m_title.c_str(); }
  const char* Dir(void) const { return m_directory.c_str(); }
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  bool ParseLine(const char *s);
};

#endif //__TIMERS_H
