/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// C++ Implementation: karaokelyricstextlrc

#include <math.h>

#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include "karaokelyricstextustar.h"


CKaraokeLyricsTextUStar::CKaraokeLyricsTextUStar( const CStdString & lyricsFile )
  : CKaraokeLyricsText()
{
  m_lyricsFile = lyricsFile;
}


CKaraokeLyricsTextUStar::~CKaraokeLyricsTextUStar()
{
}


std::vector< CStdString > CKaraokeLyricsTextUStar::readFile(const CStdString & lyricsFile, bool report_errors )
{
  std::vector< CStdString > lines;

  XFILE::CFile file;

  if ( !file.Open( lyricsFile ) )
    return std::vector< CStdString >();

  unsigned int lyricSize = (unsigned int) file.GetLength();

  if ( !lyricSize )
  {
    if ( report_errors )
      CLog::Log( LOGERROR, "UStar lyric loader: lyric file %s has zero length", lyricsFile.c_str() );

    return std::vector< CStdString >();
  }

  // Read the file into memory array
  std::vector<char> lyricData( lyricSize );

  file.Seek( 0, SEEK_SET );

  // Read the whole file
  if ( file.Read( &lyricData[0], lyricSize) != lyricSize )
    return std::vector< CStdString >(); // disk error?

  file.Close();

  // Parse into the string array
  unsigned int offset = 0;
  unsigned int lineoffset = 0;

  while ( offset < lyricSize )
  {
    // End of line?
    if ( lyricData[offset] == 0x0D || lyricData[offset] == 0x0A )
    {
      // An empty line?
      if ( lineoffset != offset )
      {
        lyricData[offset] = '\0';
        lines.push_back( &lyricData[lineoffset] );
      }

      // Point to the next symbol
      lineoffset = offset + 1;
    }

    offset++;
  }

  // Last line, if any
  if ( lineoffset < lyricSize )
  {
    lyricData[lyricSize-1] = '\0';
    lines.push_back( &lyricData[lineoffset] );
  }

  return lines;
}


bool CKaraokeLyricsTextUStar::isValidFile(const CStdString & lyricsFile)
{
  std::vector< CStdString > lines = readFile( lyricsFile, false );

  if ( lines.size() == 0 )
    return false;

  // Generally the first line should start with '#' and the last line should start with 'E'
  return ( lines[0][0] == '#' && lines[lines.size()-1][0] == 'E' );
}


bool CKaraokeLyricsTextUStar::Load()
{
  // Header parameters
  CStdString coverimage, bgimage;
  int bpm = 0, startoffsetms = 0;
  bool relative = false;

  // Read the text file
  std::vector< CStdString > lines = readFile( m_lyricsFile, true );

  if ( lines.size() == 0 )
    return false;

  // Clear the lyrics array
  clearLyrics();

  // Parse and validate the header according to 
  // http://ultrastardeluxe.xtremeweb-hosting.net/wiki/doku.php?id=editor:txt_file
  unsigned int idx = 0;

  for ( ; idx < lines.size() && lines[idx][0] == '#'; idx++ )
  {
    // Parse into key:value
    int offset = lines[idx].Find( ':' );

    if ( offset == -1 )
    {
      CLog::Log( LOGERROR, "UStar lyric loader: invalid line '%s', no semicolon", lines[idx].c_str() );
      return false;
    }

    CStdString key = lines[idx].Mid( 1, offset - 1 );
    CStdString value = lines[idx].Mid( offset + 1 );

    if ( key == "TITLE" )
      m_songName = value;
    else if ( key == "ARTIST" )
      m_artist = value;
    else if ( key == "VIDEO" )
    {
      CStdString videopath;
      URIUtils::GetDirectory( m_lyricsFile, videopath );
      m_videoFile = videopath + value;

      if ( !XFILE::CFile::Exists( m_videoFile ) )
      {
        CLog::Log( LOGERROR, "UStar lyric loader: VIDEO entry is present, but video file %s is not found", m_videoFile.c_str() );
        m_videoFile.clear();
      }
    }
    else if ( key == "COVER" )
      coverimage = value;
    else if ( key == "BACKGROUND" )
      bgimage = value;
    else if ( key == "VIDEOGAP" )
      m_videoOffset = atoi( value.c_str() );
    else if ( key == "BPM" )
      bpm = atoi( value.c_str() );
    else if ( key == "GAP" )
      startoffsetms = atoi( value.c_str() );
    else if ( key == "RELATIVE" )
      relative = (value.ToUpper() == "YES" );
    else if ( key == "LANGUAGE" || key == "EDITION" || key == "GENRE" || key == "YEAR" || key == "MP3" )
    {
      ; // do nothing
    }
    else
      CLog::Log( LOGWARNING, "UStar lyric loader: unsupported keyword '%s'", key.c_str() );
  }

  // BPM must be defined
  if ( bpm == 0 )
  {
    CLog::Log( LOGERROR, "UStar lyric loader: BPM is not defined, file is invalid" );
    return false;
  }

  // Should be more lines
  if ( idx == lines.size() )
  {
    CLog::Log( LOGERROR, "UStar lyric loader: no lyrics found besides the header" );
    return false;
  }

  double beatstep = 60.0 / bpm / 4.0;
  CLog::Log( LOGDEBUG, "UStar lyric loader: found valid lyrics, BPM is %d (%g)", bpm, beatstep );

  // Now parse the words/notes part
  int lyric_flags = 0;

  for ( ; idx < lines.size() && lines[idx][0] != 'E'; idx++ )
  {
    char type = lines[idx][0];

    // A valid type should be followed by space
    if ( type != 'F' && type != ':' && type != '*' && type != '-' && lines[idx][1] != ' ' )
    {
      CLog::Log( LOGERROR, "UStar lyric loader: invalid line '%s', bad note type or no tail space", lines[idx].c_str() );
      return false;
    }

    // Parse the numbers in the line into the vector
    int numbercount = (type == '-') ? 1 : 3;
    char * p = &(lines[idx][1]);
    std::vector< int > numbers;

    while ( numbercount > 0 )
    {
      unsigned int length = 0;

      // Skip all leading space
      while ( isspace( *p ) )
        p++;

      // skip non-space
      while ( p[length] && !isspace( p[length] ) )
      {
        if ( !isdigit( p[length] ) )
        {
          CLog::Log( LOGERROR, "UStar lyric loader: invalid line '%s', bad digit at back-position %d", lines[idx].c_str(), numbercount );
          return false;
        }

        length++;
      }

      p[length++] = '\0';

      if ( strlen(p) == 0 )
      {
        CLog::Log( LOGERROR, "UStar lyric loader: invalid line '%s', empty digit at back-position %d", lines[idx].c_str(), numbercount );
        return false;
      }

      numbers.push_back( atoi( p ) );

      // Adjust p
      p += length;
      numbercount--;
    }

    int notestart_timing = (int)((numbers[0] * beatstep) * 10 + (startoffsetms / 100));

    if ( type != '-' )
    {
      // Pitch is not used yet; notelenght will not be used at all
      //int notelength = numbers[1] * beatstep * 10;
      //int notepitch = numbers[2];

      addLyrics( p, notestart_timing, lyric_flags | LYRICS_CONVERT_UTF8 );
      lyric_flags = 0;

      //CLog::Log( LOGDEBUG, ":: %d %d [%d - %d] %d '%s'", numbers[0], numbers[1], notestart_timing, notelength, notepitch, text );
    }
    else
    {
      lyric_flags = CKaraokeLyricsText::LYRICS_NEW_LINE;
      addLyrics( " ", notestart_timing, lyric_flags | LYRICS_CONVERT_UTF8 );

      // If we're relative, adjust to the pause start
      if ( relative )
        startoffsetms += (int)((numbers[0] * beatstep) * 10);

      //CLog::Log( LOGERROR, ":: [stop] %d [%d]", numbers[0], notestart_timing );
    }
  }

  // Indicate that lyrics have pitch
  m_hasPitch = true;
  return true;
}
