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

// C++ Implementation: karaokelyricstextkar

#include "utils/CharsetConverter.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/Utf8Utils.h"
#include <math.h>

#include "karaokelyricstextkar.h"


// Parsed lyrics
typedef struct
{
  unsigned int  clocks;
  unsigned int  track;
  std::string    text;
  unsigned int  flags;

} MidiLyrics;


// Parsed tempo change structure
typedef struct
{
  unsigned int  clocks;
  unsigned int  tempo;

} MidiTempo;


// Parsed per-channel info
typedef struct
{
  unsigned int  total_lyrics;
  unsigned int  total_lyrics_space;

} MidiChannelInfo;


// Based entirely on class MidiTimestamp from pyKaraoke
// Based entirely on class MidiTimestamp from pyKaraoke
class MidiTimestamp
{
  private:
    const std::vector<MidiTempo>&   m_tempo;
    double               m_currentMs;
    unsigned int          m_currentClick;
    unsigned int          m_tempoIndex;
    unsigned int          m_division;

  public:
    MidiTimestamp( const std::vector<MidiTempo>& tempo, unsigned int division )
      : m_tempo (tempo), m_division (division)
    {
      reset();
    }

    void reset()
    {
      m_currentMs = 0.0;
      m_currentClick = 0;
      m_tempoIndex = 0;
    }

    double getTimeForClicks( unsigned int click, unsigned int tempo )
    {
      double microseconds = ( ( float(click) / m_division ) * tempo );
      return microseconds / 1000.0;
    }

    // Returns the "advanced" clock value in ms.
    double advanceClocks( unsigned int click )
    {
      // Moves time forward to the indicated click number.
      if ( m_currentClick > click )
        throw("Malformed lyrics timing");

      unsigned int clicks = click - m_currentClick;

      while ( clicks > 0 && m_tempoIndex < m_tempo.size() )
      {
        // How many clicks remain at the current tempo?
        unsigned int clicksRemaining = 0;

        if ( m_tempo[ m_tempoIndex ].clocks - m_currentClick > 0 )
          clicksRemaining = m_tempo[ m_tempoIndex ].clocks - m_currentClick;

        unsigned int clicksUsed = clicks < clicksRemaining ? clicks : clicksRemaining;

        if ( clicksUsed > 0 && m_tempoIndex > 0 )
          m_currentMs += getTimeForClicks( clicksUsed, m_tempo[ m_tempoIndex - 1 ].tempo );

        m_currentClick += clicksUsed;
        clicks -= clicksUsed;
        clicksRemaining -= clicksUsed;

        if ( clicksRemaining == 0 )
          m_tempoIndex++;
      }

      if ( clicks > 0 )
      {
        // We have reached the last tempo mark of the song, so this tempo holds forever.
        m_currentMs += getTimeForClicks( clicks, m_tempo[ m_tempoIndex - 1 ].tempo );
        m_currentClick += clicks;
      }

      return m_currentMs;
    }
};



CKaraokeLyricsTextKAR::CKaraokeLyricsTextKAR( const std::string & midiFile )
  : CKaraokeLyricsText()
{
  m_midiFile = midiFile;
}


CKaraokeLyricsTextKAR::~CKaraokeLyricsTextKAR()
{
}


bool CKaraokeLyricsTextKAR::Load()
{
  XFILE::CFile file;
  bool succeed = true;
  m_reportedInvalidVarField = false;

  // Clear the lyrics array
  clearLyrics();

  if (file.LoadFile(m_midiFile, m_midiData) <= 0)
    return false;

  file.Close();

  // Parse MIDI
  try
  {
    parseMIDI();
  }
  catch ( const char * p )
  {
    CLog::Log( LOGERROR, "KAR lyrics loader: cannot load file: %s", p );
    succeed = false;
  }

  m_midiData.clear();
  return succeed;
}


//
// Got a lot of good ideas from pykaraoke by Kelvin Lawson (kelvinl@users.sf.net). Thanks!
//
void CKaraokeLyricsTextKAR::parseMIDI()
{
  m_midiOffset = 0;

  // Bytes 0-4: header
  unsigned int header = readDword();

  // If we get MS RIFF header, skip it
  if ( header == 0x52494646 )
  {
    setPos( currentPos() + 16 );
    header = readDword();
  }

  // MIDI header
  if ( header != 0x4D546864 )
    throw( "Not a MIDI file" );

  // Bytes 5-8: header length
  unsigned int header_length = readDword();

  // Bytes 9-10: format
  unsigned short format = readWord();

  if ( format > 2 )
    throw( "Unsupported format" );

  // Bytes 11-12: tracks
  unsigned short tracks = readWord();

  // Bytes 13-14: divisious
  unsigned short divisions = readWord();

  if ( divisions > 32768 )
    throw( "Unsupported division" );

  // Number of tracks is always 1 if format is 0
  if ( format == 0 )
    tracks = 1;

  // Parsed per-channel info
  std::vector<MidiLyrics> lyrics;
  std::vector<MidiTempo> tempos;
  std::vector<MidiChannelInfo> channels;

  channels.resize( tracks );

  // Set up default tempo
  MidiTempo te;
  te.clocks = 0;
  te.tempo = 500000;
  tempos.push_back( te );

  int preferred_lyrics_track = -1;
  int lastchannel = 0;
  int laststatus = 0;
  unsigned int firstNoteClocks = 1000000000; // arbitrary large value
  unsigned int next_line_flag = 0;

  // Point to first byte after MIDI header
  setPos( 8 + header_length );

  // Parse all tracks
  for ( int track = 0; track < tracks; track++ )
  {
    char tempbuf[1024];
    unsigned int clocks = 0;

    channels[track].total_lyrics = 0;
    channels[track].total_lyrics_space = 0;

    // Skip malformed files
    if ( readDword() != 0x4D54726B )
      throw( "Malformed track header" );

    // Next track position
    int tracklen = readDword();
    unsigned int nexttrackstart = tracklen + currentPos();

    // Parse track until end of track event
    while ( currentPos() < nexttrackstart )
    {
      // field length
      clocks += readVarLen();
      unsigned char msgtype = readByte();

      //
      // Meta event
      //
      if ( msgtype == 0xFF )
      {
        unsigned char metatype = readByte();
        unsigned int metalength = readVarLen();

        if ( metatype == 3 )
        {
          // Track title metatype
          if ( metalength >= sizeof( tempbuf ) )
            throw( "Meta event too long" );

          readData( tempbuf, metalength );
          tempbuf[metalength] = '\0';

          if ( !strcmp( tempbuf, "Words" ) )
            preferred_lyrics_track = track;
        }
        else if ( metatype == 5 || metatype == 1 )
        {
          // Lyrics metatype
          if ( metalength >= sizeof( tempbuf ) )
            throw( "Meta event too long" );

          readData( tempbuf, metalength );
          tempbuf[metalength] = '\0';

          if ( (tempbuf[0] == '@' && tempbuf[1] >= 'A' && tempbuf[1] <= 'Z')
          || strstr( tempbuf, " SYX" ) || strstr( tempbuf, "Track-" )
          || strstr( tempbuf, "%-" ) || strstr( tempbuf, "%+" ) )
          {
            // Keywords
            if ( tempbuf[0] == '@' && tempbuf[1] == 'T' && strlen( tempbuf + 2 ) > 0 )
            {
              if ( m_songName.empty() )
                m_songName = convertText( tempbuf + 2 );
              else
              {
                if ( !m_artist.empty() )
                  m_artist += "[CR]";

                m_artist += convertText( tempbuf + 2 );
              }
            }
          }
          else
          {
            MidiLyrics lyric;
            lyric.clocks = clocks;
            lyric.track = track;
            lyric.flags = next_line_flag;

            if ( tempbuf[0] == '\\' )
            {
              lyric.flags = CKaraokeLyricsText::LYRICS_NEW_PARAGRAPH;
              lyric.text = convertText( tempbuf + 1 );
            }
            else if ( tempbuf[0] == '/' )
            {
              lyric.flags = CKaraokeLyricsText::LYRICS_NEW_LINE;
              lyric.text = convertText( tempbuf + 1 );
            }
            else if ( tempbuf[1] == '\0' && (tempbuf[0] == '\n' || tempbuf[0] == '\r' ) )
            {
              // An empty line; do not add it but set the flag
              if ( next_line_flag == CKaraokeLyricsText::LYRICS_NEW_LINE )
                next_line_flag = CKaraokeLyricsText::LYRICS_NEW_PARAGRAPH;
              else
                next_line_flag = CKaraokeLyricsText::LYRICS_NEW_LINE;
            }
            else
            {
              next_line_flag = (strchr(tempbuf, '\n') || strchr(tempbuf, '\r')) ? CKaraokeLyricsText::LYRICS_NEW_LINE : CKaraokeLyricsText::LYRICS_NONE;
              lyric.text = convertText( tempbuf );
            }

            lyrics.push_back( lyric );

            // Calculate the number of spaces in current syllable
            for ( unsigned int j = 0; j < metalength; j++ )
            {
              channels[ track ].total_lyrics++;

              if ( tempbuf[j] == 0x20 )
                channels[ track ].total_lyrics_space++;
            }
          }
        }
        else if ( metatype == 0x51 )
        {
          // Set tempo event
          if ( metalength != 3 )
            throw( "Invalid tempo" );

          unsigned char a1 = readByte();
          unsigned char a2 = readByte();
          unsigned char a3 = readByte();
          unsigned int tempo = (a1 << 16) | (a2 << 8) | a3;

          // MIDI spec says tempo could only be on the first track...
          // but some MIDI editors still put it on second. Shouldn't break anything anyway, but let's see
          //if ( track != 0 )
          //  throw( "Invalid tempo track" );

          // Check tempo array. If previous tempo has higher clocks, abort.
          if ( tempos.size() > 0 && tempos[ tempos.size() - 1 ].clocks > clocks )
            throw( "Invalid tempo" );

          // If previous tempo has the same clocks value, override it. Otherwise add new.
          if ( tempos.size() > 0 && tempos[ tempos.size() - 1 ].clocks == clocks )
            tempos[ tempos.size() - 1 ].tempo = tempo;
          else
          {
            MidiTempo mt;
            mt.clocks = clocks;
            mt.tempo = tempo;

            tempos.push_back( mt );
          }
        }
        else
        {
          // Skip the event completely
          setPos( currentPos() + metalength );
        }
      }
      else if ( msgtype== 0xF0 || msgtype == 0xF7 )
      {
        // SysEx event
        unsigned int length = readVarLen();
        setPos( currentPos() + length );
      }
      else
      {
        // Regular MIDI event
        if ( msgtype & 0x80 )
        {
          // Status byte
          laststatus = ( msgtype >> 4) & 0x07;
          lastchannel = msgtype & 0x0F;

          if ( laststatus != 0x07 )
            msgtype = readByte() & 0x7F;
        }

        switch ( laststatus )
        {
          case 0:  // Note off
            readByte();
            break;

          case 1: // Note on
            if ( (readByte() & 0x7F) != 0 ) // this would be in fact Note off
            {
              // Remember the time the first note played
              if ( firstNoteClocks > clocks )
                firstNoteClocks = clocks;
            }
            break;

          case 2: // Key Pressure
          case 3: // Control change
          case 6: // Pitch wheel
            readByte();
            break;

          case 4: // Program change
          case 5: // Channel pressure
            break;

          default: // case 7: Ignore this event
            if ( (lastchannel & 0x0F) == 2 ) // Sys Com Song Position Pntr
              readWord();
            else if ( (lastchannel & 0x0F) == 3 ) // Sys Com Song Select(Song #)
              readByte();
            break;
        }
      }
    }
  }

  // The MIDI file is parsed. Now try to find the preferred lyric track
  if ( preferred_lyrics_track == -1 || channels[preferred_lyrics_track].total_lyrics == 0 )
  {
    unsigned int max_lyrics = 0;

    for ( unsigned int t = 0; t < tracks; t++ )
    {
      if ( channels[t].total_lyrics > max_lyrics )
      {
        preferred_lyrics_track = t;
        max_lyrics = channels[t].total_lyrics;
      }
    }
  }

  if ( preferred_lyrics_track == -1 )
    throw( "No lyrics found" );

  // We found the lyrics track. Dump some debug information.
  MidiTimestamp mts( tempos, divisions );
  double firstNoteTime = mts.advanceClocks( firstNoteClocks );

  CLog::Log( LOGDEBUG, "KAR lyric loader: found lyric track %d, first offset %d (%g ms)", preferred_lyrics_track, firstNoteClocks, firstNoteTime );

  // Now go through all lyrics on this track, convert them into time.
  mts.reset();

  for ( unsigned int i = 0; i < lyrics.size(); i++ )
  {
    if ( (int) lyrics[i].track != preferred_lyrics_track )
      continue;

    double lyrics_timing = mts.advanceClocks( lyrics[i].clocks );

    // Skip lyrics which start before the first note
    if ( lyrics_timing < firstNoteTime )
      continue;

    unsigned int mstime = (unsigned int)ceil( (lyrics_timing - firstNoteTime) / 100);
    addLyrics( lyrics[i].text, mstime, lyrics[i].flags );
  }
}


unsigned char CKaraokeLyricsTextKAR::readByte()
{
  if (m_midiOffset >= m_midiData.size())
    throw( "Cannot read byte: premature end of file" );

  return (unsigned char) m_midiData.get()[m_midiOffset++];
}

unsigned short CKaraokeLyricsTextKAR::readWord()
{
  if (m_midiOffset + 1 >= m_midiData.size())
    throw( "Cannot read word: premature end of file" );

  m_midiOffset += 2;
  return ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-2])) << 8 |
         ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-1]));
}


unsigned int CKaraokeLyricsTextKAR::readDword()
{
  if (m_midiOffset + 3 >= m_midiData.size())
    throw( "Cannot read dword: premature end of file" );

  m_midiOffset += 4;
  return ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-4])) << 24 |
         ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-3])) << 16 |
         ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-2])) << 8 |
         ((unsigned int)((unsigned char)m_midiData.get()[m_midiOffset-1]));
}

int CKaraokeLyricsTextKAR::readVarLen()
{
  int l = 0, c;

  c = readByte();

  if ( !(c & 0x80) )
    return l | c;

  l = (l | (c & 0x7f)) << 7;
  c = readByte();

  if ( !(c & 0x80) )
    return l | c;

  l = (l | (c & 0x7f)) << 7;
  c = readByte();

  if ( !(c & 0x80) )
    return l | c;

  l = (l | (c & 0x7f)) << 7;
  c = readByte();

  if ( !(c & 0x80) )
    return l | c;

  if ( !m_reportedInvalidVarField )
  {
    m_reportedInvalidVarField = true;
    CLog::Log( LOGWARNING, "Warning: invalid MIDI file, workaround enabled but MIDI might not sound as expected" );
  }

  l = (l | (c & 0x7f)) << 7;
  c = readByte();

  if ( !(c & 0x80) )
    return l | c;

  throw( "Cannot read variable field" );
}

unsigned int CKaraokeLyricsTextKAR::currentPos() const
{
  return m_midiOffset;
}

void CKaraokeLyricsTextKAR::setPos(unsigned int offset)
{
  m_midiOffset = offset;
}

void CKaraokeLyricsTextKAR::readData(void * buf, unsigned int length)
{
  for ( unsigned int i = 0; i < length; i++ )
    *((char*)buf + i) = readByte();
}

std::string CKaraokeLyricsTextKAR::convertText( const char * data )
{
  std::string strUTF8;

  // Use some heuristics; need to replace by real detection stuff later
  if (CUtf8Utils::isValidUtf8(data) || CSettings::Get().GetString("karaoke.charset") == "DEFAULT")
    strUTF8 = data;
  else
    g_charsetConverter.ToUtf8( CSettings::Get().GetString("karaoke.charset"), data, strUTF8 );

  if ( strUTF8.size() == 0 )
    strUTF8 = " ";

  return strUTF8;
}
