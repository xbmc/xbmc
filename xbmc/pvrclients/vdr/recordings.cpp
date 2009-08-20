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

/*
 * This code is taken from recordings.c in the Video Disk Recorder ('VDR')
 */

#include "recordings.h"

cRecording::cRecording()
{
  m_channelName     = NULL;
  m_aux             = NULL;
  m_title           = NULL;
  m_shortText       = NULL;
  m_description     = NULL;
  m_fileName        = NULL;
  m_framesPerSecond = DEFAULTFRAMESPERSECOND;
  m_priority        = MAXPRIORITY;
  m_lifetime        = MAXLIFETIME;
  m_EventID         = 0;
  m_StartTime       = 0;
  m_Duration        = 0;
  m_TableID         = 0;
  m_Version         = 0xFF;
  m_vps             = 0;
  m_Index           = -1;
}

cRecording::cRecording(const PVR_RECORDINGINFO *Recording)
{

}

cRecording::~cRecording()
{
  free(m_aux);
  free(m_channelName);
  free(m_title);
  free(m_shortText);
  free(m_description);
  free(m_fileName);

}

bool cRecording::ParseLine(const char *s)
{
  char *t = skipspace(s + 1);
  switch (*s) 
  {
    case 'C': 
      {
        char *p = strchr(t, ' ');
        if (p) 
        {
          free(m_channelName);
          m_channelName = strdup(compactspace(p));
          *p = 0; // strips optional channel name
        }
  //      if (*t)
  //        channelID = tChannelID::FromString(t);
      }
      return true;
    case 'D': 
      strreplace(t, '|', '\n');
      SetDescription(t);
      return true;
    case 'E': 
      {
        unsigned int EventID;
        time_t StartTime;
        int Duration;
        unsigned int TableID = 0;
        unsigned int Version = 0xFF;
        int n = sscanf(t, "%u %ld %d %X %X", &EventID, &StartTime, &Duration, &TableID, &Version);
        if (n >= 3 && n <= 5) {
          m_EventID   = EventID;
          m_StartTime = StartTime;
          m_Duration  = Duration;
          m_TableID   = TableID;
          m_Version   = Version;
        }
      }
      return true;
    case 'F': 
      m_framesPerSecond = atof(t);
      return true;
    case 'L': 
      m_lifetime = atoi(t);
      return true;
    case 'P': 
      m_priority = atoi(t);
      return true;
    case 'S': 
      SetShortText(t);
      return true;
    case 'T': 
      SetTitle(t);
      return true;
    case 'V': 
      m_vps = atoi(t);
      return true;
    case '@': 
      free(m_aux);
      m_aux = strdup(t);
      return true;
    case '#': 
      return true; // comments are ignored


    default:
      break;
  }
  return false;
}

bool cRecording::ParseEntryLine(const char *s)
{
  if (*s >= '0' && *s <= '9')
  {
    char recdate[256];
    char rectime[256];
    char rectext[1024];

    if (sscanf(s, " %u %[^ ] %[^ ] %[^\n]", &m_Index, recdate, rectime, rectext) >= 3)
    {
      m_fileName = strcpyrealloc(m_fileName, rectext);
    }
    return true;
  }
  
  return false;
}

//    case 'X': if (!components)
//                 components = new cComponents;
//              components->SetComponent(components->NumComponents(), t);
//              break;


void cRecording::SetFramesPerSecond(double FramesPerSecond)
{
  m_framesPerSecond = FramesPerSecond;
}

void cRecording::SetTitle(const char *Title)
{
  m_title = strcpyrealloc(m_title, Title);
}

void cRecording::SetShortText(const char *ShortText)
{
  m_shortText = strcpyrealloc(m_shortText, ShortText);
}

void cRecording::SetDescription(const char *Description)
{
  m_description = strcpyrealloc(m_description, Description);
}
