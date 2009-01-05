//
// C++ Implementation: karaokelyricstextlrc
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"

#include "Util.h"
#include "FileSystem/File.h"
#include "Settings.h"

#include "karaokelyricstextlrc.h"


CKaraokeLyricsTextLRC::CKaraokeLyricsTextLRC( const CStdString & lyricsFile )
	: CKaraokeLyricsText()
{
	m_lyricsFile = lyricsFile;
}


CKaraokeLyricsTextLRC::~CKaraokeLyricsTextLRC()
{
}

bool CKaraokeLyricsTextLRC::Load()
{
	enum ParserState
	{
		PARSER_INIT,		// looking for time
 		PARSER_IN_TIME,		// processing time
		PARSER_IN_LYRICS	// processing lyrics
	};

	XFILE::CFile file;

	// Clear the lyrics array
	clearLyrics();
			
	if ( !file.Open( m_lyricsFile, TRUE ) )
		return false;

	unsigned int lyricSize = (unsigned int) file.GetLength();

	if ( !lyricSize )
	{
		CLog::Log( LOGERROR, "LRC lyric loader: lyric file %s has zero length", m_lyricsFile.c_str() );
		return false;
	}

	// Read the file into memory array
	std::vector<char> lyricData( lyricSize );

	file.Seek( 0, SEEK_SET );

	// Read the whole file
	if ( file.Read( lyricData.data(), lyricSize) != lyricSize )
		return false; // disk error? 
	
	file.Close();

	// Parse the correction value
	int timing_correction = (int) round( g_advancedSettings.m_karaokeSyncDelayLRC * 10 );
	
	//
	// A simple state machine to parse the file
	//
	ParserState state = PARSER_INIT;
	unsigned int state_offset = 0;
	unsigned int lyric_flags = 0;
	int lyric_time = -1;
	char * p = lyricData.data();

	CStdString ext, songfilename = getSongFile();
	CUtil::GetExtension( songfilename, ext );
	
	for ( unsigned int offset = 0; offset < lyricSize; offset++, p++ )
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
				if ( lyric_time == -1 )
				{
					CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no time before lyrics" );
					return false;
				}
				
				// Add existing lyrics
				char current = *p;
				CStdString text;
				
				if ( offset > state_offset )
				{
					// null-terminate string, we saved current char anyway
					*p = '\0';
					text = lyricData.data() + state_offset;
				}
				else
					text = " "; // add a single space for empty lyric

				// If this was end of line, set the flags accordingly
				if ( current == '\n' )
				{
					// Add space after the trailing lyric in lrc
					text += " ";
					addLyrics( text, lyric_time, lyric_flags );
					state_offset = -1;
					lyric_flags = CKaraokeLyricsText::LYRICS_NEW_LINE;
					state = PARSER_INIT;
				}
				else
				{
					// No conversion needed as the file should be in UTF8 already
					addLyrics( text, lyric_time, lyric_flags );
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
				int mins, secs, tenths;

				if ( offset == state_offset )
				{
					CLog::Log( LOGERROR, "LRC lyric loader: empty time" );
					return false; // [] - empty time
				}

				// null-terminate string
				char * timestr = lyricData.data() + state_offset;
				*p = '\0';

				// Now check if this is time field
				if ( timestr[0] >= 'a' && timestr[0] <= 'z' && timestr[1] >= 'a' && timestr[1] <= 'z' )
				{
					// This is title/artist field.
					if ( !strncmp( timestr, "ar:", 3 ) )
						m_artist = timestr + 3;
					else if ( !strncmp( timestr, "ti:", 3 ) )
						m_songName = timestr + 3;

					state_offset = -1;
					state = PARSER_INIT;
					continue;
				}
				else if ( sscanf( timestr, "%d:%d.%d", &mins, &secs, &tenths ) == 3 )
					lyric_time = mins * 600 + secs * 10 + tenths;
				else if ( sscanf( timestr, "%d:%d", &mins, &secs ) == 2 )
					lyric_time = mins * 600 + secs * 10;
				else
				{
					// bad time
					CLog::Log( LOGERROR, "LRC lyric loader: lyrics file has no proper time field: '%s'", timestr );
					return false;
				}

				// Correct timing if necessary
				lyric_time += timing_correction;

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
