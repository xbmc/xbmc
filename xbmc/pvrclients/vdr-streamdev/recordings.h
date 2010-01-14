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

#ifndef __RECORDINGS_H
#define __RECORDINGS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tools.h"

#define DEFAULTFRAMESPERSECOND 25.0
#define MAXPRIORITY 99
#define MAXLIFETIME 99

class cRecording
{
private:
  int m_Index;
  char *m_channelName;
  char *m_fileName;
  char *m_directory;
  double m_framesPerSecond;
  int m_priority;
  int m_lifetime;
  char *m_aux;
  unsigned int m_EventID;
  time_t m_StartTime;
  int m_Duration;
  unsigned int m_TableID;
  unsigned int m_Version;
  char *m_title;             // Title of this event
  char *m_shortText;         // Short description of this event (typically the episode name in case of a series)
  char *m_description;       // Description of this event
  time_t m_vps;              // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)

public:
  cRecording(const PVR_RECORDINGINFO *Recording);
  cRecording();
  virtual ~cRecording();

  bool ParseLine(const char *s);
  bool ParseEntryLine(const char *s);
  const char *ChannelName(void) const { return m_channelName; }
  const char *Aux(void) const { return m_aux; }
  double FramesPerSecond(void) const { return m_framesPerSecond; }
  void SetFramesPerSecond(double m_FramesPerSecond);
  int Index(void) const { return m_Index; }
  int Priority(void) const { return m_priority; }
  int Lifetime(void) const { return m_lifetime; }
  time_t StartTime(void) const { return m_StartTime; }
  time_t Duration(void) const { return m_Duration; }
  time_t Vps(void) const { return m_vps; }
  const char *Title(void) const { return m_title; }
  const char *ShortText(void) const { return m_shortText; }
  const char *Description(void) const { return m_description; }
  const char *FileName(void) const { return m_fileName; }
  const char *Directory(void) const { return m_directory; }
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
  void SetDescription(const char *Description);
};

#endif //__RECORDINGS_H
