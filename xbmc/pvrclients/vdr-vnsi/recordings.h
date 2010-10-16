#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
#define MAXPRIORITY       99
#define MAXLIFETIME       99
#define RECEXT            ".rec"
#define DELEXT            ".del"
#define DATAFORMATPES     "%4d-%02d-%02d.%02d%*c%02d.%02d.%02d" RECEXT
#define NAMEFORMATPES     "%s/%s/" "%4d-%02d-%02d.%02d.%02d.%02d.%02d" RECEXT
#define DATAFORMATTS      "%4d-%02d-%02d.%02d.%02d.%d-%d" RECEXT
#define NAMEFORMATTS      "%s/%s/" DATAFORMATTS
#define RESUMEFILESUFFIX  "/resume%s%s"
#define SUMMARYFILESUFFIX "/summary.vdr"
#define INFOFILESUFFIX    "/info"
#define MARKSFILESUFFIX   "/marks"

class cRecording
{
private:
  int m_Index;
  int m_resume;
  int m_fileSizeMB;
  char *m_channelName;
  char *m_fileName;
  char *m_directory;
  CStdString m_stream_url;
  double m_framesPerSecond;
  int m_priority;
  int m_lifetime;
  char *m_aux;
  unsigned int m_EventID;
  time_t m_StartTime;
  int m_Duration;
  unsigned int m_TableID;
  unsigned int m_Version;
  bool m_isPesRecording;
  bool m_deleted;
  char *m_title;             // Title of this event
  char *m_shortText;         // Short description of this event (typically the episode name in case of a series)
  char *m_description;       // Description of this event
  time_t m_vps;              // Video Programming Service timestamp (VPS, aka "Programme Identification Label", PIL)
  bool Read(FILE *f);

public:
  cRecording(const char *FileName, const char *BaseDir);
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
  const char *StreamURL(void) const { return m_stream_url.c_str(); }
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
  void SetDescription(const char *Description);
};

#endif //__RECORDINGS_H
