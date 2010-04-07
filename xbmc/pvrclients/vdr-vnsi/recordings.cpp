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

/*
 * This code is taken from recordings.c in the Video Disk Recorder ('VDR')
 */

#include "StdString.h"
#include "recordings.h"
#include "client.h"

#define RESUME_NOT_INITIALIZED (-2)

cRecording::cRecording()
{
  m_channelName     = NULL;
  m_aux             = NULL;
  m_title           = NULL;
  m_shortText       = NULL;
  m_description     = NULL;
  m_fileName        = NULL;
  m_directory       = NULL;
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
  m_isPesRecording  = false;
  m_resume          = RESUME_NOT_INITIALIZED;
  m_fileSizeMB      = -1; // unknown
  m_deleted         = false;
}

cRecording::cRecording(const PVR_RECORDINGINFO *Recording)
{

}

cRecording::cRecording(const char *FileName, const char *BaseDir)
{
  m_channelName     = NULL;
  m_aux             = NULL;
  m_title           = NULL;
  m_shortText       = NULL;
  m_description     = NULL;
  m_fileName        = NULL;
  m_directory       = NULL;
  m_resume          = RESUME_NOT_INITIALIZED;
  m_fileSizeMB      = -1; // unknown
  m_priority        = MAXPRIORITY;
  m_lifetime        = MAXLIFETIME;
  m_isPesRecording  = false;
  m_framesPerSecond = DEFAULTFRAMESPERSECOND;
  m_deleted         = false;

  FileName = m_fileName = strdup(FileName);
  if (*(m_fileName + strlen(m_fileName) - 1) == '/')
     *(m_fileName + strlen(m_fileName) - 1) = 0;
  FileName += strlen(BaseDir) + 1;
  const char *p = strrchr(FileName, '/');
  if (p)
  {
    time_t now = time(NULL);
    struct tm tm_r;
    struct tm t = *localtime_r(&now, &tm_r); // this initializes the time zone in 't'
    t.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
    int channel;
    int instanceId;
    if (7 == sscanf(p + 1, DATAFORMATTS, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &channel, &instanceId)
        || 7 == sscanf(p + 1, DATAFORMATPES, &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &m_priority, &m_lifetime))
    {
      t.tm_year -= 1900;
      t.tm_mon--;
      t.tm_sec = 0;
      m_StartTime = mktime(&t);
      char *path = MALLOC(char, p - FileName + 1);
      strncpy(path, FileName, p - FileName);
      path[p - FileName] = 0;

      /* Make some default title based upon directory names */
      p = strrchr(path, '/');
      if (p)
        m_title = strcpyrealloc(m_title, p + 1);
      else
        m_title = strcpyrealloc(m_title, path);

      m_directory = MALLOC(char, strlen(path) - strlen(m_title) + 1);
      strncpy(m_directory, path, strlen(path) - strlen(m_title));
      m_directory[strlen(path) - strlen(m_title)] = 0;

      if (g_bCharsetConv)
      {
        CStdString str_result = m_directory;
        XBMC->UnknownToUTF8(str_result);
        m_directory = strcpyrealloc(m_directory, str_result.c_str());
      }

      m_isPesRecording = instanceId < 0;
      m_stream_url.Format("%s/%s", m_fileName, m_isPesRecording ? "*.vdr" : "*.ts");
      free(path);
    }
    else
      return;
    // read info file:
    CStdString InfoFileName;
    InfoFileName.Format("%s%s", m_fileName, m_isPesRecording ? INFOFILESUFFIX ".vdr" : INFOFILESUFFIX);
    FILE *f = fopen(InfoFileName.c_str(), "r");
    if (f)
    {
      if (!Read(f))
        XBMC->Log(LOG_ERROR, "EPG data problem in file %s", InfoFileName.c_str());
      fclose(f);
    }
    else if (errno != ENOENT)
      XBMC->Log(LOG_ERROR, "ERROR (%s,%d,s): %m", __FILE__, __LINE__, InfoFileName.c_str());
  }
}

cRecording::~cRecording()
{
  free(m_aux);
  free(m_channelName);
  free(m_title);
  free(m_shortText);
  free(m_description);
  free(m_fileName);
  free(m_directory);
}

bool cRecording::Read(FILE *f)
{
  cReadLine ReadLine;
  char *s;
  int line = 0;
  while ((s = ReadLine.Read(f)) != NULL)
  {
    ++line;
    CStdString str_result = s;
    if (g_bCharsetConv)
      XBMC->UnknownToUTF8(str_result);
    if (!ParseLine(str_result.c_str()))
      return false;
  }
  return true;
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
        if (n >= 3 && n <= 5)
        {
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
      CStdString fileName = rectext;
      fileName.Replace('/', '_');
      fileName.Replace('\\', '_');
      fileName.Replace('?', '_');
#if defined(_WIN32) || defined(_WIN64)
      // just filter out some illegal characters on windows
      fileName.Replace(':', '_');
      fileName.Replace('*', '_');
      fileName.Replace('?', '_');
      fileName.Replace('\"', '_');
      fileName.Replace('<', '_');
      fileName.Replace('>', '_');
      fileName.Replace('|', '_');
      fileName.TrimRight(".");
      fileName.TrimRight(" ");
#endif
      size_t found = fileName.find_last_of("~");
      if (found != CStdString::npos)
      {
        CStdString dir = fileName.substr(0,found);
        dir.Replace('~','/');
        m_directory = strcpyrealloc(m_directory, dir.c_str());
        m_fileName = strcpyrealloc(m_fileName, fileName.substr(found+1).c_str());
      }
      else
      {
        m_fileName = strcpyrealloc(m_fileName, fileName.c_str());
        m_directory = strcpyrealloc(m_directory, "");
      }
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
