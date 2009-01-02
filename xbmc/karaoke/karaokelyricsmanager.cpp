//
// C++ Implementation: karaokelyricsmanager
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
#include "GUIWindowManager.h"

#include "karaokelyrics.h"
#include "karaokelyricsfactory.h"
#include "karaokelyricsmanager.h"
#include "karaokesongselector.h"


CKaraokeLyricsManager::CKaraokeLyricsManager()
{
	m_Lyrics = 0;
	m_songSelector = 0;
	m_karaokeSongPlaying = false;
}

CKaraokeLyricsManager::~ CKaraokeLyricsManager()
{
	if ( m_Lyrics )
	{
		m_Lyrics->Shutdown();
		delete m_Lyrics;
	}
	
	delete m_songSelector;
}

bool CKaraokeLyricsManager::Start(const CStdString & strSongPath)
{
	CSingleLock lock (m_CritSection);

	// We only need one song selector
	if ( !m_songSelector )
		m_songSelector = new CKaraokeSongSelector();
	
	if ( m_Lyrics )
		Stop();	// shouldn't happen, but...

	m_Lyrics = CKaraokeLyricsFactory::CreateLyrics( strSongPath );

	if ( !m_Lyrics )
	{
		CLog::Log( LOGDEBUG, "Karaoke: lyrics for song %s not found", strSongPath.c_str() );
		return false;
	}

	m_Lyrics->initData( strSongPath );
	
	// Load the lyrics
	if ( !m_Lyrics->Load() )
	{
		CLog::Log( LOGWARNING, "Karaoke: lyrics for song %s found but cannot be loaded", strSongPath.c_str() );
		delete m_Lyrics;
		m_Lyrics = 0;
		return false;
	}

	CLog::Log( LOGDEBUG, "Karaoke: lyrics for song %s loaded successfully", strSongPath.c_str() );
	m_Lyrics->InitGraphics();
	m_Lyrics->initStartTime();

	// make sure we have fullscreen viz
	if ( m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION )
		m_gWindowManager.ActivateWindow( WINDOW_VISUALISATION );

	m_karaokeSongPlaying = true;
	return true;
}

void CKaraokeLyricsManager::Stop()
{
	CSingleLock lock (m_CritSection);

	m_karaokeSongPlaying = false;
	
	if ( !m_Lyrics )
		return;
	
	m_Lyrics->Shutdown();
	delete m_Lyrics;
	m_Lyrics = 0;
}


void CKaraokeLyricsManager::Render()
{
	CSingleLock lock (m_CritSection);

	if ( m_Lyrics )
		m_Lyrics->Render();
	
	if ( m_songSelector )
		m_songSelector->Render();
}


bool CKaraokeLyricsManager::isLyricsAvailable() const
{
	CSingleLock lock (m_CritSection);
	
	return m_Lyrics != 0;
}

bool CKaraokeLyricsManager::OnAction(const CAction & action)
{
	CSingleLock lock (m_CritSection);
	
	if ( !m_Lyrics || !m_karaokeSongPlaying )
		return false;
	CLog::Log( LOGERROR, "action %d", action.wID );
	switch(action.wID)
	{
		case REMOTE_0:
		case REMOTE_1:
		case REMOTE_2:
		case REMOTE_3:
		case REMOTE_4:
		case REMOTE_5:
		case REMOTE_6:
		case REMOTE_7:
		case REMOTE_8:
		case REMOTE_9:
          	// Offset from key codes back to button number
			if ( m_songSelector )
			{
				m_songSelector->OnButtonNumeric( action.wID - REMOTE_0 );
				return true;
			}
			break;
/*
		case ACTION_SELECT_ITEM:
			if ( m_songSelector )
			{
				m_songSelector->OnButtonSelect();
				return true;
			}
			break;
*/			
		case ACTION_SUBTITLE_DELAY_MIN:
			m_Lyrics->lyricsDelayDecrease();
			return true;
			
		case ACTION_SUBTITLE_DELAY_PLUS:
			m_Lyrics->lyricsDelayIncrease();
			return true;
	}
	
	return false;
}
