/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// C++ Implementation: karaokelyricstextlrc

#include <math.h>

#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include "karaokelyricstextlrc.h"

enum ParserState
{
  PARSER_INIT,    // looking for time
  PARSER_IN_TIME,    // processing time
  PARSER_IN_LYRICS  // processing lyrics
};

// Used in multi-time lyric loader
typedef struct
{
  std::string    text;
  unsigned int   timing;
  unsigned int   flags;
} MtLyric;
 
CKaraokeLyricsTextLRC::CKaraokeLyricsTextLRC( const std::string & lyricsFile )
  : CKaraokeLyricsText()
{
  m_lyricsFile = lyricsFile;
}


CKaraokeLyricsTextLRC::~CKaraokeLyricsTextLRC()
{
}

bool CKaraokeLyricsTextLRC::Load()
{
  XFILE::CFile file;

  // Clear the lyrics array
  clearLyrics();

  XFILE::auto_buffer buf;
  if (file.LoadFile(m_lyricsFile, buf) <= 0)
  {
    CLog::Log(LOGERROR, "%s: can't load \"%s\" file", __FUNCTION__, m_lyricsFile.c_str());
    return false;
  }

  file.Close();

  // Parse the correction value
  int timing_correction = MathUtils::round_int( g_advancedSettings.m_karaokeSyncDelayLRC * 10 );

  unsigned int offset = 0;

  std::string songfilename = getSongFile();

  // Skip windoze UTF8 file prefix, if any, and reject UTF16 files
  if (buf.size() > 3)
  {
    if ((unsigned char)buf.get()[0] == 0xFF && (unsigned char)buf.get()[1] == 0xFE)
    {
      CLog::Log( LOGERROR, "LRC lyric loader: lyrics file is in UTF16 encoding, must be in UTF8" );
      return false;
    }

    // UTF8 prefix added by some windoze apps
    if ((unsigned char)buf.get()[0] == 0xEF && (unsigned char)buf.get()[1] == 0xBB && (unsigned char)buf.get()[2] == 0xBF)
      offset = 3;
  }

  if (checkMultiTime(buf.get() + offset, buf.size() - offset))
    return ParserMultiTime(buf.get() + offset, buf.size() - offset, timing_correction);
  else
    return ParserNormal(buf.get() + offset, buf.size() - offset, timing_correction);
}

bool CKaraokeLyricsTextLRC::checkMultiTime(char *lyricData, unsigned int lyricSize)
{
  // return true only when find lines like:
  // [02:24][01:40][00:51][00:05]I'm a big big girl
  // but not like:
  // [00:01.10]I [00:01.09]just [00:01.50]call
  bool inTime = false;
  bool newLine = true;
  bool maybeMultiTime = false;
  unsigned int i = 0;
  for ( char * p = lyricData; i < lyricSize; i++, p++ )
  {
    if (inTime)
    {
      if (*p == ']')
        inTime = false;
    }
    else
    {
      if (*p == '[')
      {
        inTime = true;
        if (newLine)
        {
          newLine = false;
        }
        else
        {
          if (*(p - 1) != ']')
            return false;
          else
            maybeMultiTime = true;
        }
      }
      if (*p == '\n')
        newLine = true;
    }
  }
  return maybeMultiTime;
}

bool CKaraokeLyricsTextLRC::ParserNormal(char *lyricData, unsigned int lyricSize, int timing_correction)
{
  CLog::Log( LOGDEBUG, "LRC lyric loader: parser normal lyrics file" );
  //
  // A simple state machine to parse the file
  //
  ParserState state = PARSER_INIT;
  unsigned int state_offset = 0;
  unsigned int lyric_flags = 0;
  int lyric_time = -1;
  int start_offset = 0;
  unsigned int offset = 0;

  for ( char * p = lyricData; offset < lyricSize; offset++, p++ )
  {
    // Skip \r
    if ( *p == 0x0D )
      continue;

    if ( state == PARSER_IN_LYRICS )
    {
      // Lyrics are terminated either by \n or by [
      if ( *p == '\n' || *p == '[' || *p == '<' )
      {
        // Time must be there
        if ( lyric_time == -1 )
        {
          CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no time before lyrics" );
          return false;
        }

        // Add existing lyrics
        char current = *p;
        std::string text;

        if ( offset > state_offset )
        {
          // null-terminate string, we saved current char anyway
          *p = '\0';
          text = &lyricData[0] + state_offset;
        }
        else
          text = " "; // add a single space for empty lyric

        // If this was end of line, set the flags accordingly
        if ( current == '\n' )
        {
          // Add space after the trailing lyric in lrc
          text += " ";
          addLyrics( text, lyric_time, lyric_flags | LYRICS_CONVERT_UTF8 );
          state_offset = -1;
          lyric_flags = CKaraokeLyricsText::LYRICS_NEW_LINE;
          state = PARSER_INIT;
        }
        else
        {
          // No conversion needed as the file should be in UTF8 already
          addLyrics( text, lyric_time, lyric_flags | LYRICS_CONVERT_UTF8 );
          lyric_flags = 0;
          state_offset = offset + 1;
          state = PARSER_IN_TIME;
        }

        lyric_time = -1;
      }
    }
    else if ( state == PARSER_IN_TIME )
    {
      // Time is terminated by ] or >
      if ( *p == ']' || *p == '>' )
      {
        int mins, secs, htenths, ltenths = 0;

        if ( offset == state_offset )
        {
          CLog::Log( LOGERROR, "LRC lyric loader: empty time" );
          return false; // [] - empty time
        }

        // null-terminate string
        char * timestr = &lyricData[0] + state_offset;
        *p = '\0';

        // Now check if this is time field or info tag. Info tags are like [ar:Pink Floyd]
        char * fieldptr = strchr( timestr, ':' );
        if ( timestr[0] >= 'a' && timestr[0] <= 'z' && timestr[1] >= 'a' && timestr[1] <= 'z' && fieldptr )
        {
          // Null-terminate the field name and switch to the field value
          *fieldptr = '\0';
          fieldptr++;

          while ( isspace( *fieldptr ) )
            fieldptr++;

          // Check the info field
          if ( !strcmp( timestr, "ar" ) )
            m_artist += fieldptr;
          else if ( !strcmp( timestr, "sr" ) )
          {
            // m_artist += "[CR]" + std::string( fieldptr ); // Add source to the artist name as a separate line
          }
          else if ( !strcmp( timestr, "ti" ) )
            m_songName = fieldptr;
          else if ( !strcmp( timestr, "offset" ) )
          {
            if ( sscanf( fieldptr, "%d", &start_offset ) != 1 )
            {
              CLog::Log( LOGERROR, "LRC lyric loader: invalid [offset:] value '%s'", fieldptr );
              return false; // [] - empty time
            }

            // Offset is in milliseconds; convert to 1/10 seconds
            start_offset /= 100;
          }

          state_offset = -1;
          state = PARSER_INIT;
          continue;
        }
        else if ( sscanf( timestr, "%d:%d.%1d%1d", &mins, &secs, &htenths, &ltenths ) == 4 )
          lyric_time = mins * 600 + secs * 10 + htenths + MathUtils::round_int( ltenths / 10 );
        else if ( sscanf( timestr, "%d:%d.%1d", &mins, &secs, &htenths ) == 3 )
          lyric_time = mins * 600 + secs * 10 + htenths;
        else if ( sscanf( timestr, "%d:%d", &mins, &secs ) == 2 )
          lyric_time = mins * 600 + secs * 10;
        else
        {
          // bad time
          CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no proper time field: '%s'", timestr );
          return false;
        }

        // Correct timing if necessary
        lyric_time += start_offset;
        lyric_time += timing_correction;

        if ( lyric_time < 0 )
          lyric_time = 0;

        // Set to next char
        state_offset = offset + 1;
        state = PARSER_IN_LYRICS;
      }
    }
    else if ( state == PARSER_INIT )
    {
      // Ignore spaces
      if ( *p == ' ' || *p == '\t' )
        continue;

      // We're looking for [ or <
      if ( *p == '[' || *p == '<' )
      {
        // Set to next char
        state_offset = offset + 1;
        state = PARSER_IN_TIME;
        lyric_time = -1;
      }
      else if ( *p == '\n' )
      {
        // If we get a newline and we're not paragraph, set it
        if ( lyric_flags & CKaraokeLyricsText::LYRICS_NEW_LINE )
          lyric_flags = CKaraokeLyricsText::LYRICS_NEW_PARAGRAPH;
      }
      else
      {
        // Everything else is error
        CLog::Log( LOGERROR, "LRC lyric loader: lyrics file does not start from time" );
        return false;
      }
    }
  }
  return true;
}

bool CKaraokeLyricsTextLRC::ParserMultiTime(char *lyricData, unsigned int lyricSize, int timing_correction)
{
  CLog::Log( LOGDEBUG, "LRC lyric loader: parser mult-time lyrics file" );
  ParserState state = PARSER_INIT;
  unsigned int state_offset = 0;
  unsigned int lyric_flags = 0;
  std::vector<int> lyric_time(1, -1);
  int time_num = 0;
  std::vector<MtLyric>  mtline;
  MtLyric line;
  int start_offset = 0;
  unsigned int offset = 0;

  for ( char * p = lyricData; offset < lyricSize; offset++, p++ )
  {
    // Skip \r
    if ( *p == 0x0D )
      continue;

    if ( state == PARSER_IN_LYRICS )
    {
      // Lyrics are terminated either by \n or by [
      if ( *p == '\n' || *p == '[' )
      {
        // Time must be there
        if ( lyric_time[0] == -1 )
        {
          CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no time before lyrics" );
          return false;
        }

        // Add existing lyrics
        char current = *p;
        std::string text;

        if ( offset > state_offset )
        {
          // null-terminate string, we saved current char anyway
          *p = '\0';
          text = &lyricData[0] + state_offset;
        }
        else
          text = " "; // add a single space for empty lyric

        // If this was end of line, set the flags accordingly
        if ( current == '\n' )
        {
          // Add space after the trailing lyric in lrc
          text += " ";
          for ( int i = 0; i <= time_num; i++ )
          {
            line.text = text;
            line.flags = lyric_flags | LYRICS_CONVERT_UTF8;
            line.timing = lyric_time[i];
            mtline.push_back( line );
          }
          state_offset = -1;
          lyric_flags = CKaraokeLyricsText::LYRICS_NEW_LINE;
          state = PARSER_INIT;
        }
        else
        {
          // No conversion needed as the file should be in UTF8 already
          for ( int i = 0; i <= time_num; i++ )
          {
            line.text = text;
            line.flags = lyric_flags | LYRICS_CONVERT_UTF8;
            line.timing = lyric_time[i];
            mtline.push_back( line );
          }
          lyric_flags = 0;
          state_offset = offset + 1;
          state = PARSER_IN_TIME;
        }

        time_num = 0;
        lyric_time.resize(1);
        lyric_time[0] = -1;
      }
    }
    else if ( state == PARSER_IN_TIME )
    {
      // Time is terminated by ] or >
      if ( *p == ']' || *p == '>' )
      {
        int mins, secs, htenths, ltenths = 0;

        if ( offset == state_offset )
        {
          CLog::Log( LOGERROR, "LRC lyric loader: empty time" );
          return false; // [] - empty time
        }

        // null-terminate string
        char * timestr = &lyricData[0] + state_offset;
        *p = '\0';

        // Now check if this is time field or info tag. Info tags are like [ar:Pink Floyd]
        char * fieldptr = strchr( timestr, ':' );
        if ( timestr[0] >= 'a' && timestr[0] <= 'z' && timestr[1] >= 'a' && timestr[1] <= 'z' && fieldptr )
        {
          // Null-terminate the field name and switch to the field value
          *fieldptr = '\0';
          fieldptr++;

          while ( isspace( *fieldptr ) )
            fieldptr++;

          // Check the info field
          if ( !strcmp( timestr, "ar" ) )
            m_artist += fieldptr;
          else if ( !strcmp( timestr, "sr" ) )
          {
            // m_artist += "[CR]" + std::string( fieldptr ); // Add source to the artist name as a separate line
          }
          else if ( !strcmp( timestr, "ti" ) )
            m_songName = fieldptr;
          else if ( !strcmp( timestr, "offset" ) )
          {
            if ( sscanf( fieldptr, "%d", &start_offset ) != 1 )
            {
              CLog::Log( LOGERROR, "LRC lyric loader: invalid [offset:] value '%s'", fieldptr );
              return false; // [] - empty time
            }

            // Offset is in milliseconds; convert to 1/10 seconds
            start_offset /= 100;
          }

          state_offset = -1;
          state = PARSER_INIT;
          continue;
        }
        else if ( sscanf( timestr, "%d:%d.%1d%1d", &mins, &secs, &htenths, &ltenths ) == 4 )
          lyric_time[time_num] = mins * 600 + secs * 10 + htenths + MathUtils::round_int( ltenths / 10 );
        else if ( sscanf( timestr, "%d:%d.%1d", &mins, &secs, &htenths ) == 3 )
          lyric_time[time_num] = mins * 600 + secs * 10 + htenths;
        else if ( sscanf( timestr, "%d:%d", &mins, &secs ) == 2 )
          lyric_time[time_num] = mins * 600 + secs * 10;
        else
        {
          // bad time
          CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no proper time field: '%s'", timestr );
          return false;
        }

        // Correct timing if necessary
        lyric_time[time_num] += start_offset;
        lyric_time[time_num] += timing_correction;

        if ( lyric_time[time_num] < 0 )
          lyric_time[time_num] = 0;

        // Multi-time line
        if ( *(p + 1) == '[' )
        {
          offset++;
          p++;
          state_offset = offset + 1;
          state = PARSER_IN_TIME;
          time_num++;
          lyric_time.push_back(-1);
        }
        else
        {
          // Set to next char
          state_offset = offset + 1;
          state = PARSER_IN_LYRICS;
        }
      }
    }
    else if ( state == PARSER_INIT )
    {
      // Ignore spaces
      if ( *p == ' ' || *p == '\t' )
        continue;

      // We're looking for [ or <
      if ( *p == '[' || *p == '<' )
      {
        // Set to next char
        state_offset = offset + 1;
        state = PARSER_IN_TIME;

        time_num = 0;
        lyric_time.resize(1);
        lyric_time[0] = -1;
      }
      else if ( *p == '\n' )
      {
        // If we get a newline and we're not paragraph, set it
        if ( lyric_flags & CKaraokeLyricsText::LYRICS_NEW_LINE )
          lyric_flags = CKaraokeLyricsText::LYRICS_NEW_PARAGRAPH;
      }
      else
      {
        // Everything else is error
        CLog::Log( LOGERROR, "LRC lyric loader: lyrics file does not start from time" );
        return false;
      }
    }
  }

  unsigned int lyricsNum = mtline.size();
  if ( lyricsNum >= 2 )
  {
    for ( unsigned int i = 0; i < lyricsNum - 1; i++ )
    {
      for ( unsigned int j = i + 1; j < lyricsNum; j++ )
      {
        if ( mtline[i].timing > mtline[j].timing )
        {
          line = mtline[i];
          mtline[i] = mtline[j];
          mtline[j] = line;
        }
      }
    }
  }
  for ( unsigned int i=0; i < lyricsNum; i++ )
    addLyrics( mtline[i].text, mtline[i].timing, mtline[i].flags );

  return true;
}

