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

#ifndef __RECORDINGS_H
#define __RECORDINGS_H

#include <stdlib.h>
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

using namespace std;

#define DEFAULTFRAMESPERSECOND 25.0
#define MAXPRIORITY 99
#define MAXLIFETIME 99

class cRecording
{
private:
  int m_Index;
  string m_channelName;
  string m_fileName;
  string m_stream;
  string m_lifetime;
  time_t m_StartTime;
  int m_Duration;
  string m_title;             // Title of this event
  string m_shortText;         // Short description of this event (typically the episode name in case of a series)
  string m_description;       // Description of this event
  //time_t m_vps;               // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
  time_t m_UTCdiff;

public:
  cRecording(const PVR_RECORDINGINFO *Recording);
  cRecording();
  virtual ~cRecording();

  bool ParseLine(const std::string& data);
  const char *ChannelName(void) const { return m_channelName.c_str(); }
  int Index(void) const { return m_Index; }
  time_t StartTime(void) const { return m_StartTime; }
  time_t Duration(void) const { return m_Duration; }
  const char *Title(void) const { return m_title.c_str(); }
  const char *Description(void) const { return m_description.c_str(); }
  const char *FileName(void) const { return m_fileName.c_str(); }
  const char *Stream(void) const { return m_stream.c_str(); }
};

#endif //__RECORDINGS_H
