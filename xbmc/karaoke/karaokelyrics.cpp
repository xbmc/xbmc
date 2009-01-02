//
// C++ Implementation: karaokelyrics
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

#include "karaokelyrics.h"

CKaraokeLyrics::CKaraokeLyrics()
{
	m_avDelay = 0;
}


CKaraokeLyrics::~CKaraokeLyrics()
{
}

void CKaraokeLyrics::Shutdown()
{
}

bool CKaraokeLyrics::InitGraphics()
{
	return true;
}

void CKaraokeLyrics::initData( const CStdString & songPath )
{
	// Reset AV delay
	m_avDelay = 0;
	m_songPath = songPath;
}

void CKaraokeLyrics::lyricsDelayIncrease()
{
	CLog::Log( LOGERROR, "delay increased %g", m_avDelay);
	m_avDelay += 0.25; // 250ms
}

void CKaraokeLyrics::lyricsDelayDecrease()
{
	CLog::Log( LOGERROR, "delay decreased %g",m_avDelay);
	m_avDelay -= 0.25; // 250ms
}

double CKaraokeLyrics::getSongTime() const
{
	// m_avDelay may be negative
	double songtime = g_application.GetTime() - m_songLyricsStartTime + m_avDelay;
	return songtime >= 0 ? songtime : 0.0;
}

CStdString CKaraokeLyrics::getSongFile() const
{
	return m_songPath;
}


void CKaraokeLyrics::initStartTime()
{
	m_songLyricsStartTime = g_application.GetTime();
}
