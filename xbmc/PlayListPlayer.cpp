#include "PlayListPlayer.h"
#include "application.h"
#include "util.h"

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
	m_iCurrentSong=0;
	m_bChanged=false;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
	m_PlaylistMusic.Clear();
	m_PlaylistMusicTemp.Clear();
	m_PlaylistVideo.Clear();
	m_PlaylistVideoTemp.Clear();
}

void CPlayListPlayer::PlayNext(bool bAutoPlay)
{
	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	m_iCurrentSong++;
	if (m_iCurrentSong >= playlist.size() )
		m_iCurrentSong=0;

	if (bAutoPlay)
	{
		const CPlayList::CPlayListItem& item = playlist[m_iCurrentSong];
		if ( CUtil::IsShoutCast(item.GetFileName()) )
		{
			return;
		}
	}
	Play(m_iCurrentSong);
}

void CPlayListPlayer::PlayPrevious()
{
	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	m_iCurrentSong--;
	if (m_iCurrentSong < 0)
		m_iCurrentSong=playlist.size()-1;

	Play(m_iCurrentSong);

}

void CPlayListPlayer::Play(int iSong)
{
	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	if (iSong < 0) iSong=0;
	if (iSong >= playlist.size() ) iSong=playlist.size()-1;

	m_bChanged=true;
	m_iCurrentSong=iSong;
	CPlayList::CPlayListItem item = playlist[m_iCurrentSong];

	if ( !CUtil::IsShoutCast(item.GetFileName()) && !CUtil::IsCDDA(item.GetFileName()) )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_PLAY_NEXT_PREV, 0, 0, GetCurrentPlaylist(), 0, (LPVOID)&item );
		m_gWindowManager.SendMessage( msg );
	}
	g_application.PlayFile( item.GetFileName() );	
}

void CPlayListPlayer::SetCurrentSong(int iSong)
{
	m_iCurrentSong=iSong;
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

int CPlayListPlayer::GetCurrentPlaylist()
{
	return m_iCurrentPlayList;
}

void CPlayListPlayer::SetCurrentPlaylist( int iPlayList )
{
	m_iCurrentPlayList = iPlayList;
	m_bChanged=true;
}

CPlayList& CPlayListPlayer::GetPlaylist( int nPlayList)
{
	switch ( nPlayList )
	{
	case PLAYLIST_MUSIC:
		return m_PlaylistMusic;
		break;
	case PLAYLIST_MUSIC_TEMP:
		return m_PlaylistMusicTemp;
		break;
	case PLAYLIST_VIDEO:
		return m_PlaylistVideo;
		break;
	case PLAYLIST_VIDEO_TEMP:
		return m_PlaylistVideoTemp;
		break;
	default:
		return m_PlaylistMusic;
		break;
	}
}

int CPlayListPlayer::RemoveDVDItems()
{
	int nRemovedM = m_PlaylistMusic.RemoveDVDItems();
	m_PlaylistMusicTemp.RemoveDVDItems();
	int nRemovedV = m_PlaylistVideo.RemoveDVDItems();
	m_PlaylistVideoTemp.RemoveDVDItems();

	return nRemovedM + nRemovedV;
}

void CPlayListPlayer::Shuffle()
{
	m_PlaylistMusic.Shuffle();
	m_iCurrentSong=-1;
}

