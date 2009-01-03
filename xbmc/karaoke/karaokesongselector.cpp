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
#include "MusicDatabase.h"

#include "karaokesongselector.h"


static const unsigned int INACTIVITY_TIME = 5000;	// 5 secs
static const unsigned int MAX_SONG_ID = 100000;


CKaraokeSongSelector::CKaraokeSongSelector()
{
	m_selectorActive = false;
	m_inactiveTime = 0;
	m_selectedNumber = 0;
	
	m_songSelected = false;
}

CKaraokeSongSelector::~ CKaraokeSongSelector()
{
}

void CKaraokeSongSelector::OnButtonNumeric(unsigned int code)
{
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
			CStdString path = m_karaokeSong.strFileName;
			CFileItemPtr pItem( new CFileItem( path, false) );
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
	
	CMusicDatabase musicdatabase;
	if ( musicdatabase.Open() )
	{
		m_songSelected = musicdatabase.GetSongByKaraokeNumber( m_selectedNumber, m_karaokeSong );
		musicdatabase.Close();
		
		if ( m_songSelected )
			m_selectorMessage += " " + m_karaokeSong.strTitle;
		else
			m_selectorMessage += " (not found)";
	}
	else
	{
		m_songSelected = false;
		m_selectorMessage += " (Database error)";
	}
}
