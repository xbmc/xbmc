//
// C++ Implementation: cguikaraokesongselector
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
#include "Application.h"
#include "GUITextLayout.h"
#include "GUIFontManager.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "PlayList.h"

#include "karaokesongselector.h"

static const unsigned int INACTIVITY_TIME = 5000;	// 5 secs
static const unsigned int MAX_SONG_ID = 100000;


CKaraokeSongSelector::CKaraokeSongSelector()
{
	m_selectorActive = false;
	m_inactiveTime = 0;
	m_selectedNumber = 0;
	
	m_dataLoaded = false;
	m_songSelected = false;
	
	loadDatabase();
}

CKaraokeSongSelector::~ CKaraokeSongSelector()
{
}

void CKaraokeSongSelector::OnButtonNumeric(unsigned int code)
{
	if ( !m_dataLoaded )
		return;
	
	if ( !m_selectorActive )
	{
		m_songSelected = false;
		m_selectorActive = true;
		m_selectedNumber = 0;
	}

	// Add the number
	m_selectedNumber = m_selectedNumber * 10 + code;
	
	// If overflow (a typical way to delete the old number is add zeros), handle it
	if ( m_selectedNumber >= MAX_SONG_ID )
		m_selectedNumber %= MAX_SONG_ID;

	m_inactiveTime = timeGetTime() + INACTIVITY_TIME;
	updateOnScreenMessage();
}

void CKaraokeSongSelector::OnButtonSelect()
{
	// If "select" pressed, pop up the info window
	if ( !m_selectorActive )
	{
		m_selectorActive = true;
		m_selectedNumber = 0;
		m_inactiveTime = timeGetTime() + INACTIVITY_TIME;
	}
	else
	{
		// Add the song into queue
		if ( m_songSelected )
		{
			CStdString path = m_karaokeDatabase[ m_selectedNumber ];
			CFileItemPtr pItem( new CFileItem(path, false) );
			g_playlistPlayer.Add( PLAYLIST_MUSIC, pItem );
			
			CLog::Log(LOGDEBUG, "Karaoke song selector: adding song %s [%d]", path.c_str(), m_selectedNumber);
		}

		m_selectorActive = false;
		m_selectedNumber = 0;
	}
}

void CKaraokeSongSelector::Render()
{
	if ( !m_selectorActive )
		return;
	
	if ( timeGetTime() >= m_inactiveTime )
	{
		// Temporary fix until I know how to do it properly
		OnButtonSelect();

		m_songSelected = false;
		m_selectorActive = false;
		return;
	}
	
	float x = 10;
	float y = 10;
	
	CGUITextLayout::DrawOutlineText( g_fontManager.GetFont("font13"), x, y, 0xffffffff, 0xff000000, 2, m_selectorMessage );
}

void CKaraokeSongSelector::updateOnScreenMessage()
{
	CStdString message;
	message.Format( "%06d", m_selectedNumber );
	
	m_selectorMessage = "Song: " + message;
	
	// FIXME: query the song from the real database
	std::map< unsigned int, CStdString >::const_iterator it = m_karaokeDatabase.find( m_selectedNumber );
	
	if ( it != m_karaokeDatabase.end() )
	{
		m_songSelected = true;
		CStdString filename = CUtil::GetFileName( it->second );
		CUtil::RemoveExtension( filename );
		m_selectorMessage += " " + filename;
	}
	else
	{
		m_songSelected = false;
		m_selectorMessage += " (not found)";
	}
}

void CKaraokeSongSelector::loadDatabase()
{
	XFILE::CFile file;

	if ( !file.Open( _P("Q:\\karaoke.db"), TRUE ) )
	{
		CLog::Log( LOGERROR, "Cannot read karaoke database" );
		return;
	}

	unsigned int size = (unsigned int) file.GetLength();

	if ( !size )
		return;

	// Read the file into memory array
	std::vector<char> data( size + 1 );

	file.Seek( 0, SEEK_SET );

	// Read the whole file
	if ( file.Read( data.data(), size) != size )
		return; // disk error? 
	
	file.Close();
	data[ size ] = '\0';

	//
	// A simple state machine to parse the file
	//
	CStdString fileroot;
	char * linestart = data.data();
	
	for ( char * p = data.data(); *p; p++ )
	{
		// Skip \r
		if ( *p == 0x0D )
			continue;

		// Line number or ROOT
		if ( *p == 0x0A )
		{
			*p = '\0';
			
			if ( !strncmp( linestart, "ROOT\t", 5 ) )
			{
				fileroot = linestart + 5;
			}
			else
			{
				unsigned int num;
				if ( sscanf( linestart, "%05d\t", &num ) != 1 )
				{
					CLog::Log( LOGERROR, "Error in line %s", linestart );
					return;
				}
				
				if ( fileroot )
					m_karaokeDatabase[ num ] = fileroot + "/" + (linestart + 6);
				else
					m_karaokeDatabase[ num ] = linestart + 6;
					
			}
			
			linestart = p + 1;
		}
	}
	
	m_dataLoaded = true;
}
