#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "StdString.h"

enum eTimerFlags { tfNone      = 0x0000,
                   tfActive    = 0x0001,
                   tfInstant   = 0x0002,
                   tfVps       = 0x0004,
                   tfRecording = 0x0008,
                   tfAll       = 0xFFFF,
                 };

class cTimer
{
private:
  mutable time_t startTime, stopTime;
  time_t lastSetEvent;
  bool recording, pending, inVpsMargin;
  unsigned int flags;
  mutable time_t day; ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
  int weekdays;       ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
  int start;
  int stop;
  int priority;
  int channel;
  int lifetime;
  mutable char file[256];
  mutable char name[256];
  mutable char dir[256];
  int index;
  char *aux;

public:
  cTimer(const PVR_TIMERINFO *Timer);
  cTimer();
  virtual ~cTimer();

  int Index(void) const { return index; }
  bool Recording(void) const { return recording; }
  bool Pending(void) const { return pending; }
  bool InVpsMargin(void) const { return inVpsMargin; }
  unsigned int Flags(void) const { return flags; }
  unsigned int Channel(void) const { return channel; }
  time_t Day(void) const { return day; }
  int WeekDays(void) const { return weekdays; }
  int Start(void) const { return start; }
  int Stop(void) const { return stop; }
  int Priority(void) const { return priority; }
  int Lifetime(void) const { return lifetime; }
  time_t FirstDay(void) const { return weekdays ? day : 0; }
  const char *Aux(void) const { return aux; }
  const char *File(void) const { return file; }
  const char *Title(void) const { return name; }
  const char *Dir(void) const { return dir; }
  bool Matches(time_t t = 0, bool Directly = false, int Margin = 0) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;
  bool Parse(const char *s);
  bool IsSingleEvent(void) const;
  static bool ParseDay(const char *s, time_t &Day, int &WeekDays);
  static int GetMDay(time_t t);
  static int GetWDay(time_t t);
  bool DayMatches(time_t t) const;
  static time_t IncDay(time_t t, int Days);
  static time_t SetTime(time_t t, int SecondsFromMidnight);
  void ClrFlags(unsigned int Flags);
  void InvFlags(unsigned int Flags);
  bool HasFlags(unsigned int Flags) const;
  static int TimeToInt(int t);
  CStdString ToText() const;
  static CStdString PrintDay(time_t Day, int WeekDays);
};

#endif //__TIMERS_H
