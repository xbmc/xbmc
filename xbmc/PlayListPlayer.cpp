#include "PlayListPlayer.h"
#include "application.h"

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
	m_strPlayListName="";
	m_iCurrentSong=0;
	m_bChanged=false;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
	Clear();
}

void CPlayListPlayer::PlayNext()
{
	if (size() <= 0) return;
	m_iCurrentSong++;
	if (m_iCurrentSong >= size() )
		m_iCurrentSong=0;

	Play(m_iCurrentSong);
}

void CPlayListPlayer::PlayPrevious()
{
	if (size() <= 0) return;
	m_iCurrentSong--;
	if (m_iCurrentSong < 0)
		m_iCurrentSong=size()-1;

	Play(m_iCurrentSong);

}

void CPlayListPlayer::Play(int iSong)
{
	if (size() <= 0) return;
	if (iSong < 0) iSong=0;
	if (iSong >= size() ) iSong=size()-1;

	m_bChanged=true;
	m_iCurrentSong=iSong;
	CPlayListItem& item = m_vecItems[m_iCurrentSong];
	g_application.PlayFile( item.GetFileName() );	
}

int CPlayListPlayer::GetCurrentSong() const
{
	return m_iCurrentSong;
}

bool CPlayListPlayer::HasChanged() 
{
	bool bResult=m_bChanged;
	m_bChanged=false;
	return bResult;
}
