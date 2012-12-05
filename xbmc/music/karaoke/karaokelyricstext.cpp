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

// C++ Implementation: karaokelyricstext

#include <math.h>

#include "utils/CharsetConverter.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIFont.h"
#include "karaokelyricstext.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "guilib/GUIFontManager.h"
#include "addons/Skin.h"
#include "utils/MathUtils.h"
#include "utils/log.h"

typedef struct
{
  unsigned int   text;
  unsigned int   active;
  unsigned int   outline;

} LyricColors;

// Must be synchronized with strings.xml and GUISettings.cpp!
static LyricColors gLyricColors[] =
{
  // <string id="22040">white/green</string>
  // First 0xFF is alpha!
  {  0xFFDADADA,  0xFF00FF00,  0xFF000000  },

  // <string id="22041">white/red</string>
  {  0xFFDADADA,  0xFFFF0000,  0xFF000000  },

  // <string id="22042">white/blue</string>
  {  0xFFDADADA,  0xFF0000FF,  0xFF000000  },

  // <string id="22043">black/white</string>
  {  0xFF000000,  0xFFDADADA,  0xFFFFFFFF  },
};


CKaraokeLyricsText::CKaraokeLyricsText()
  : CKaraokeLyrics()
{
  m_karaokeLayout = 0;
  m_preambleLayout = 0;
  m_karaokeFont = 0;

  int coloridx = g_guiSettings.GetInt("karaoke.fontcolors");
  if ( coloridx < KARAOKE_COLOR_START || coloridx >= KARAOKE_COLOR_END )
    coloridx = 0;

  m_colorLyrics = gLyricColors[coloridx].text;
  m_colorLyricsOutline = gLyricColors[coloridx].outline;
  m_colorSinging.Format( "%08X", gLyricColors[coloridx].active );

  m_delayAfter = 50; // 5 seconds
  m_showLyricsBeforeStart = 50; // 7.5 seconds
  m_showPreambleBeforeStart = 35; // 5.5 seconds
  m_paragraphBreakTime = 50; // 5 seconds; for autodetection paragraph breaks
  m_mergeLines = true;
  m_hasPitch = false;
  m_videoOffset = 0;

  m_lyricsState = STATE_END_SONG;
}


CKaraokeLyricsText::~CKaraokeLyricsText()
{
}

void CKaraokeLyricsText::clearLyrics()
{
  m_lyrics.clear();
  m_songName.clear();
  m_artist.clear();
  m_hasPitch = false;
  m_videoFile.clear();
  m_videoOffset = 0;
}


void CKaraokeLyricsText::addLyrics(const CStdString & text, unsigned int timing, unsigned int flags, unsigned int pitch)
{
  Lyric line;

  if ( flags & LYRICS_CONVERT_UTF8 )
  {
    // Reset the flag
    flags &= ~LYRICS_CONVERT_UTF8;
    g_charsetConverter.unknownToUTF8(text, line.text);
  }
  else
  {
    line.text = text;
  }

  line.flags = flags;
  line.timing = timing;
  line.pitch = pitch;

  // If this is the first entry, remove LYRICS_NEW_LINE and LYRICS_NEW_PARAGRAPH flags
  if ( m_lyrics.size() == 0 )
    line.flags &= ~(LYRICS_NEW_LINE | LYRICS_NEW_PARAGRAPH );

  // 'New paragraph' includes new line as well
  if ( line.flags & LYRICS_NEW_PARAGRAPH )
    line.flags &= ~LYRICS_NEW_LINE;

  m_lyrics.push_back( line );
}


bool CKaraokeLyricsText::InitGraphics()
{
  if ( m_lyrics.empty() )
    return false;

  CStdString fontPath = "special://xbmc/media/Fonts/" + g_guiSettings.GetString("karaoke.font");
  m_karaokeFont = g_fontManager.LoadTTF("__karaoke__", fontPath,
                  m_colorLyrics, 0, g_guiSettings.GetInt("karaoke.fontheight"), FONT_STYLE_BOLD );
  CGUIFont *karaokeBorder = g_fontManager.LoadTTF("__karaokeborder__", fontPath,
                            m_colorLyrics, 0, g_guiSettings.GetInt("karaoke.fontheight"), FONT_STYLE_BOLD, true );

  if ( !m_karaokeFont )
  {
    CLog::Log(LOGERROR, "CKaraokeLyricsText::PrepareGraphicsData - Unable to load subtitle font");
    return false;
  }

  m_karaokeLayout = new CGUITextLayout( m_karaokeFont, true, 0, karaokeBorder );
  m_preambleLayout = new CGUITextLayout( m_karaokeFont, true, 0, karaokeBorder );

  if ( !m_karaokeLayout || !m_preambleLayout )
  {
    delete m_preambleLayout;
    delete m_karaokeLayout;
    m_karaokeLayout = m_preambleLayout = 0;

    CLog::Log(LOGERROR, "CKaraokeLyricsText::PrepareGraphicsData - cannot create layout");
    return false;
  }

  rescanLyrics();

  m_indexNextPara = 0;

  // Generate next paragraph
  nextParagraph();

  m_lyricsState = STATE_WAITING;
  return true;
}


void CKaraokeLyricsText::Shutdown()
{
  CKaraokeLyrics::Shutdown();

  delete m_preambleLayout;
  m_preambleLayout = 0;

  if ( m_karaokeLayout )
  {
    g_fontManager.Unload("__karaoke__");
    g_fontManager.Unload("__karaokeborder__");
    delete m_karaokeLayout;
    m_karaokeLayout = NULL;
  }

  m_lyricsState = STATE_END_SONG;
}


void CKaraokeLyricsText::Render()
{
  if ( !m_karaokeLayout )
    return;

  // Get the current song timing
  unsigned int songTime = (unsigned int) MathUtils::round_int( (getSongTime() * 10) );

  bool updatePreamble = false;
  bool updateText = false;

  // No returns in switch if anything needs to be drawn! Just break!
  switch ( m_lyricsState )
  {
    // the next paragraph lyrics are not shown yet. Screen is clear.
    // m_index points to the first entry.
    case STATE_WAITING:
      if ( songTime + m_showLyricsBeforeStart < m_lyrics[ m_index ].timing )
        return;

      // Is it time to play already?
      if ( songTime >= m_lyrics[ m_index ].timing )
      {
        m_lyricsState = STATE_PLAYING_PARAGRAPH;
      }
      else
      {
        m_lyricsState = STATE_PREAMBLE;
        m_lastPreambleUpdate = songTime;
      }

      updateText = true;
      break;

    // the next paragraph lyrics are shown, but the paragraph hasn't start yet.
    // Using m_lastPreambleUpdate, we redraw the marker each second.
    case STATE_PREAMBLE:
      if ( songTime < m_lyrics[ m_index ].timing )
      {
        // Time to redraw preamble?
        if ( songTime + m_showPreambleBeforeStart >= m_lyrics[ m_index ].timing )
        {
          if ( songTime - m_lastPreambleUpdate >= 10 )
          {
            // Fall through out of switch() to redraw
            m_lastPreambleUpdate = songTime;
            updatePreamble = true;
          }
        }
      }
      else
      {
        updateText = true;
        m_lyricsState = STATE_PLAYING_PARAGRAPH;
      }
      break;

    // The lyrics are shown, but nothing is colored or no color is changed yet.
    // m_indexStart, m_indexEnd and m_index are set, m_index timing shows when to color.
    case STATE_PLAYING_PARAGRAPH:
      if ( songTime >= m_lyrics[ m_index ].timing )
      {
        m_index++;
        updateText = true;

        if ( m_index > m_indexEndPara )
          m_lyricsState = STATE_END_PARAGRAPH;
      }
      break;

    // the whole paragraph is colored, but still shown, waiting until it's time to clear the lyrics.
    // m_index still points to the last entry, and m_indexNextPara points to the first entry of next
    // paragraph, or to LYRICS_END. When the next paragraph is about to start (which is
    // m_indexNextPara timing - m_showLyricsBeforeStart), the state switches to STATE_START_PARAGRAPH. When time
    // goes after m_index timing + m_delayAfter, the state switches to STATE_WAITING,
    case STATE_END_PARAGRAPH:
      {
        unsigned int paraEnd = m_lyrics[ m_indexEndPara ].timing + m_delayAfter;

        // If the next paragraph starts before current ends, use its start time as our end
        if ( m_indexNextPara != LYRICS_END && m_lyrics[ m_indexNextPara ].timing <= paraEnd + m_showLyricsBeforeStart )
        {
          if ( m_lyrics[ m_indexNextPara ].timing > m_showLyricsBeforeStart )
            paraEnd = m_lyrics[ m_indexNextPara ].timing - m_showLyricsBeforeStart;
          else
            paraEnd = 0;
        }

        if ( songTime >= paraEnd )
        {
          // Is the song ended?
          if ( m_indexNextPara != LYRICS_END )
          {
            // Are we still waiting?
            if ( songTime >= m_lyrics[ m_indexNextPara ].timing )
              m_lyricsState = STATE_PLAYING_PARAGRAPH;
            else
              m_lyricsState = STATE_WAITING;

            // Get next paragraph
            nextParagraph();
            updateText = true;
          }
          else
          {
            m_lyricsState = STATE_END_SONG;
            return;
          }
        }
      }
      break;

    case STATE_END_SONG:
      // the song is completed, there are no more lyrics to show. This state is finita la comedia.
      return;
  }

  // Calculate drawing parameters
  RESOLUTION resolution = g_graphicsContext.GetVideoResolution();
  g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
  float maxWidth = (float) g_settings.m_ResInfo[resolution].Overscan.right - g_settings.m_ResInfo[resolution].Overscan.left;

  // We must only fall through for STATE_DRAW_SYLLABLE or STATE_PREAMBLE
  if ( updateText )
  {
    // So we need to update the layout with current paragraph text, optionally colored according to index
    bool color_used = false;
    m_currentLyrics = "";

    // Draw the current paragraph test if needed
    if ( songTime + m_showLyricsBeforeStart >= m_lyrics[ m_indexStartPara ].timing )
    {
      for ( unsigned int i = m_indexStartPara; i <= m_indexEndPara; i++ )
      {
        if ( m_lyrics[i].flags & LYRICS_NEW_LINE )
          m_currentLyrics += "[CR]";

        if ( i == m_indexStartPara && songTime >= m_lyrics[ m_indexStartPara ].timing )
        {
          color_used = true;
          m_currentLyrics += "[COLOR " + m_colorSinging + "]";
        }

        if ( songTime < m_lyrics[ i ].timing && color_used )
        {
          color_used = false;
          m_currentLyrics += "[/COLOR]";
        }

        m_currentLyrics += m_lyrics[i].text;
      }

      if ( color_used )
        m_currentLyrics += "[/COLOR]";

//      CLog::Log( LOGERROR, "Updating text: state %d, time %d, start %d, index %d (time %d) [%s], text %s",
//        m_lyricsState, songTime, m_lyrics[ m_indexStartPara ].timing, m_index, m_lyrics[ m_index ].timing,
//        m_lyrics[ m_index ].text.c_str(), m_currentLyrics.c_str());
    }

    m_karaokeLayout->Update(m_currentLyrics, maxWidth * 0.9f);
    updateText = false;
  }

  if ( updatePreamble )
  {
    m_currentPreamble = "";

    // Get number of seconds left to the song start
    if ( m_lyrics[ m_indexStartPara ].timing >= songTime )
    {
      unsigned int seconds = (m_lyrics[ m_indexStartPara ].timing - songTime) / 10;

      while ( seconds-- > 0 )
        m_currentPreamble += "- ";
    }

    m_preambleLayout->Update( m_currentPreamble, maxWidth * 0.9f );
  }

  float x = maxWidth * 0.5f + g_settings.m_ResInfo[resolution].Overscan.left;
  float y = (float)g_settings.m_ResInfo[resolution].Overscan.top +
      (g_settings.m_ResInfo[resolution].Overscan.bottom - g_settings.m_ResInfo[resolution].Overscan.top) / 8;

  float textWidth, textHeight;
  m_karaokeLayout->GetTextExtent(textWidth, textHeight);
  m_karaokeLayout->RenderOutline(x, y, 0, m_colorLyricsOutline, XBFONT_CENTER_X, maxWidth);

  if ( !m_currentPreamble.IsEmpty() )
  {
    float pretextWidth, pretextHeight;
    m_preambleLayout->GetTextExtent(pretextWidth, pretextHeight);
    m_preambleLayout->RenderOutline(x - textWidth / 2, y - pretextHeight, 0, m_colorLyricsOutline, XBFONT_LEFT, maxWidth);
  }
}


void CKaraokeLyricsText::nextParagraph()
{
  if ( m_indexNextPara == LYRICS_END )
    return;

  bool new_para_found = false;
  m_indexStartPara = m_index = m_indexNextPara;

  for ( m_indexEndPara = m_index + 1; m_indexEndPara < m_lyrics.size(); m_indexEndPara++ )
  {
    if ( m_lyrics[ m_indexEndPara ].flags & LYRICS_NEW_PARAGRAPH
    || ( m_lyrics[ m_indexEndPara ].timing - m_lyrics[ m_indexEndPara - 1 ].timing ) > m_paragraphBreakTime )
    {
      new_para_found = true;
      break;
    }
  }

  // Is this the end of array?
  if ( new_para_found )
    m_indexNextPara = m_indexEndPara;
  else
    m_indexNextPara = LYRICS_END;

  m_indexEndPara--;
}


typedef struct
{
  float  width;      // total screen width of all lyrics in this line
  int    timediff;    // time difference between prev line ends and this line starts
  bool  upper_start;  // true if this line started with a capital letter
  int    offset_start;  // offset points to a 'new line' flag entry of the current line

} LyricTimingData;

void CKaraokeLyricsText::rescanLyrics()
{
  // Rescan fixes the following things:
  // - lyrics without spaces;
  // - lyrics without paragraphs
  std::vector<LyricTimingData> lyricdata;
  unsigned int spaces = 0, syllables = 0, paragraph_lines = 0, max_lines_per_paragraph = 0;

  // First get some statistics from the lyrics: number of paragraphs, number of spaces
  // and time difference between one line ends and second starts
  for ( unsigned int i = 0; i < m_lyrics.size(); i++ )
  {
    if ( m_lyrics[i].text.Find( " " ) != -1 )
      spaces++;

    if ( m_lyrics[i].flags & LYRICS_NEW_LINE )
      paragraph_lines++;

    if ( m_lyrics[i].flags & LYRICS_NEW_PARAGRAPH )
    {
      if ( max_lines_per_paragraph < paragraph_lines )
        max_lines_per_paragraph = paragraph_lines;

      paragraph_lines = 0;
    }

    syllables++;
  }

  // Second, add spaces if less than 5%, and rescan to gather more data.
  bool add_spaces = (syllables && (spaces * 100 / syllables < 5)) ? true : false;
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  float maxWidth = (float) g_settings.m_ResInfo[res].Overscan.right - g_settings.m_ResInfo[res].Overscan.left;

  CStdString line_text;
  int prev_line_idx = -1;
  int prev_line_timediff = -1;

  for ( unsigned int i = 0; i < m_lyrics.size(); i++ )
  {
    if ( add_spaces )
      m_lyrics[i].text += " ";

    // We split the lyric when it is end of line, end of array, or current string is too long already
    if ( i == (m_lyrics.size() - 1)
    || (m_lyrics[i+1].flags & (LYRICS_NEW_LINE | LYRICS_NEW_PARAGRAPH)) != 0
    || getStringWidth( line_text + m_lyrics[i].text ) >= maxWidth )
    {
      // End of line, or end of array. Add current string.
      line_text += m_lyrics[i].text;

      // Reparagraph if we're out of screen width
      if ( getStringWidth( line_text ) >= maxWidth )
        max_lines_per_paragraph = 0;

      LyricTimingData ld;
      ld.width = getStringWidth( line_text );
      ld.timediff = prev_line_timediff;
      ld.offset_start = prev_line_idx;

      // This piece extracts the first character of a new string and makes it uppercase in Unicode way
      CStdStringW temptext;
      g_charsetConverter.utf8ToW( line_text, temptext );

      // This is pretty ugly upper/lowercase for Russian unicode character set
      if ( temptext[0] >= 0x410 && temptext[0] <= 0x44F )
        ld.upper_start = temptext[0] <= 0x42F;
      else
      {
        CStdString lower = m_lyrics[i].text;
        lower.ToLower();
        ld.upper_start = (m_lyrics[i].text == lower);
      }

      lyricdata.push_back( ld );

      // Reset the params
      line_text = "";
      prev_line_idx = i + 1;
      prev_line_timediff = (i == m_lyrics.size() - 1) ? -1 : m_lyrics[i+1].timing - m_lyrics[i].timing;
    }
    else
    {
      // Handle incorrect lyrics with no line feeds in the condition statement above
      line_text += m_lyrics[i].text;
    }
  }

  // Now see if we need to re-paragraph. Basically we reasonably need a paragraph
  // to have no more than 8 lines
  if ( max_lines_per_paragraph == 0 || max_lines_per_paragraph > 8 )
  {
    // Reparagraph
    unsigned int paragraph_lines = 0;
    float total_width = 0;

    CLog::Log( LOGDEBUG, "CKaraokeLyricsText: lines need to be reparagraphed" );

    for ( unsigned int i = 0; i < lyricdata.size(); i++ )
    {
      // Is this the first line?
      if ( lyricdata[i].timediff == -1 )
      {
        total_width = lyricdata[i].width;
        continue;
      }

      // Do we merge the current line with previous? We do it if:
      // - there is a room on the screen for those lines combined
      // - the time difference between line ends and new starts is less than 1.5 sec
      // - the first character in the new line is not uppercase (i.e. new logic line)
      if ( m_mergeLines && total_width + lyricdata[i].width < maxWidth && !lyricdata[i].upper_start && lyricdata[i].timediff < 15 )
      {
        // Merge
        m_lyrics[ lyricdata[i].offset_start ].flags &= ~(LYRICS_NEW_LINE | LYRICS_NEW_PARAGRAPH);

        // Since we merged the line, add the extra space. It will be removed later if not necessary.
        m_lyrics[ lyricdata[i].offset_start ].text = " " + m_lyrics[ lyricdata[i].offset_start ].text;
        total_width += lyricdata[i].width;

//        CLog::Log(LOGERROR, "Line merged; diff %d width %g, start %d, offset %d, max %g",
//              lyricdata[i].timediff, lyricdata[i].width, lyricdata[i].upper_start, lyricdata[i].offset_start, maxWidth );
      }
      else
      {
        // Do not merge; reset width and add counter
        total_width = lyricdata[i].width;
        paragraph_lines++;

//        CLog::Log(LOGERROR, "Line not merged; diff %d width %g, start %d, offset %d, max %g",
//              lyricdata[i].timediff, lyricdata[i].width, lyricdata[i].upper_start, lyricdata[i].offset_start, maxWidth );
      }

      // Set paragraph
      if ( paragraph_lines > 3 )
      {
        m_lyrics[ lyricdata[i].offset_start ].flags &= ~LYRICS_NEW_LINE;
        m_lyrics[ lyricdata[i].offset_start ].flags |= LYRICS_NEW_PARAGRAPH;
        paragraph_lines = 0;
        line_text = "";
      }
    }
  }

  // Prepare a new first lyric entry with song name and artist.
  if ( m_songName.IsEmpty() )
  {
    m_songName = URIUtils::GetFileName( getSongFile() );
    URIUtils::RemoveExtension( m_songName );
  }

  // Split the lyrics into per-character array
  std::vector<Lyric> newlyrics;
  bool title_entry = false;

  if ( m_lyrics.size() > 0 && m_lyrics[0].timing >= 50 )
  {
    // Add a new title/artist entry
    Lyric ltitle;
    ltitle.flags = 0;
    ltitle.timing = 0;
    ltitle.text = m_songName;

    if ( !m_artist.IsEmpty() )
      ltitle.text += "[CR][CR]" + m_artist;

    newlyrics.push_back( ltitle );
    title_entry = true;
  }

  bool last_was_space = false;
  bool invalid_timing_reported = false;
  for ( unsigned int i = 0; i < m_lyrics.size(); i++ )
  {
    CStdStringW utf16;
    g_charsetConverter.utf8ToW( m_lyrics[i].text, utf16 );

    // Skip empty lyrics
    if ( utf16.size() == 0 )
      continue;

    // Use default timing for the last note
    unsigned int next_timing = m_lyrics[ i ].timing + m_delayAfter;

    if ( i < (m_lyrics.size() - 1) )
    {
      // Set the lenght for the syllable  to the length of prev syllable if:
      // - this is not the first lyric (as there is no prev otherwise)
      // - this is the last lyric on this line (otherwise use next);
      // - this is not the ONLY lyric on this line (otherwise the calculation is wrong)
      // - lyrics size is the same as previous (currently removed).
      if ( i > 0
      && m_lyrics[ i + 1 ].flags & (LYRICS_NEW_LINE | LYRICS_NEW_PARAGRAPH)
      && ! (m_lyrics[ i ].flags & (LYRICS_NEW_LINE | LYRICS_NEW_PARAGRAPH) ) )
//      && m_lyrics[ i ].text.size() == m_lyrics[ i -1 ].text.size() )
        next_timing = m_lyrics[ i ].timing + (m_lyrics[ i ].timing - m_lyrics[ i -1 ].timing );

      // Sanity check
      if ( m_lyrics[ i+1 ].timing < m_lyrics[ i ].timing )
      {
        if ( !invalid_timing_reported )
          CLog::Log( LOGERROR, "Karaoke lyrics normalizer: time went backward, enabling workaround" );

        invalid_timing_reported = true;
        m_lyrics[ i ].timing = m_lyrics[ i+1 ].timing;
      }

      if ( m_lyrics[ i+1 ].timing < next_timing )
        next_timing = m_lyrics[ i+1 ].timing;
    }

    // Calculate how many 1/10 seconds we have per lyric character
    double time_per_char = ((double) next_timing - m_lyrics[ i ].timing) / utf16.size();

    // Convert to characters
    for ( unsigned int j = 0; j < utf16.size(); j++ )
    {
      Lyric l;

      // Copy flags only to the first character
      if ( j == 0 )
        l.flags = m_lyrics[i].flags;
      else
        l.flags = 0;
      l.timing = (unsigned int) MathUtils::round_int( m_lyrics[ i ].timing + j * time_per_char );

      g_charsetConverter.wToUTF8( utf16.Mid( j, 1 ), l.text );

      if ( l.text == " " )
      {
        if ( last_was_space )
          continue;

        last_was_space = true;
      }
      else
        last_was_space = false;

      newlyrics.push_back( l );
    }
  }

  m_lyrics = newlyrics;

  // Set the NEW PARAGRAPH flag on the first real lyric entry since we changed it
  if ( title_entry )
    m_lyrics[1].flags |= LYRICS_NEW_PARAGRAPH;

  saveLyrics();
}


float CKaraokeLyricsText::getStringWidth(const CStdString & text)
{
  CStdStringW utf16;
  vecText utf32;

  g_charsetConverter.utf8ToW(text, utf16);

  utf32.resize( utf16.size() );
  for ( unsigned int i = 0; i < utf16.size(); i++ )
    utf32[i] = utf16[i];

  return m_karaokeFont->GetTextWidth(utf32);
}

void CKaraokeLyricsText::saveLyrics()
{
  XFILE::CFile file;

  CStdString out;

  for ( unsigned int i = 0; i < m_lyrics.size(); i++ )
  {
    CStdString timing;
    timing.Format( "%02d:%02d.%d", m_lyrics[i].timing / 600, (m_lyrics[i].timing % 600) / 10, (m_lyrics[i].timing % 10) );

    if ( (m_lyrics[i].flags & LYRICS_NEW_PARAGRAPH) != 0 )
      out += "\n\n";

    if ( (m_lyrics[i].flags & LYRICS_NEW_LINE) != 0 )
      out += "\n";

    out += "[" + timing + "]" + m_lyrics[i].text;
  }

  out += "\n";

  if ( !file.OpenForWrite( "special://temp/tmp.lrc", true ) )
    return;

  file.Write( out, out.size() );
}


bool CKaraokeLyricsText::HasBackground()
{
  return false;
}

bool CKaraokeLyricsText::HasVideo()
{
  return m_videoFile.IsEmpty() ? false : true;
}

void CKaraokeLyricsText::GetVideoParameters(CStdString & path, int64_t & offset)
{
  path = m_videoFile;
  offset = m_videoOffset;
}
