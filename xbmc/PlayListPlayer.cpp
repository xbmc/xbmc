
#include "stdafx.h"
#include "PlayListPlayer.h"
#include "application.h"
#include "util.h"
#include "GUIUserMessages.h"

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
	m_iCurrentSong=-1;
	m_bChanged=false;
	m_iEntriesNotFound=0;
	m_iCurrentPlayList=PLAYLIST_NONE;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
	m_PlaylistMusic.Clear();
	m_PlaylistMusicTemp.Clear();
	m_PlaylistVideo.Clear();
	m_PlaylistVideoTemp.Clear();
	m_PlaylistEmpty.Clear();
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
	switch (message.GetMessage())
	{
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			Reset();
			m_iCurrentPlayList=PLAYLIST_NONE;
		}
		break;
	}

	return true;
}

/// \brief Play next entry in current playlist
void CPlayListPlayer::PlayNext(bool bAutoPlay)
{
	if (m_iCurrentPlayList==PLAYLIST_NONE)
		return;

	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	int iSong=m_iCurrentSong;
	iSong++;

	if (iSong >= playlist.size() )
  {
		//	Is last element of video stacking playlist?
    if (m_iCurrentPlayList==PLAYLIST_VIDEO_TEMP)
		{
			//	Disable playlist playback
			m_iCurrentPlayList=PLAYLIST_NONE;
			return;
		}
		iSong=0;
  }

	if (bAutoPlay)
	{
		const CPlayList::CPlayListItem& item = playlist[iSong];
		if ( CUtil::IsShoutCast(item.GetFileName()) )
		{
			return;
		}
	}
	Play(iSong);
}

/// \brief Play previous entry in current playlist
void CPlayListPlayer::PlayPrevious()
{
	if (m_iCurrentPlayList==PLAYLIST_NONE)
		return;

	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	int iSong=m_iCurrentSong;
	iSong--;
	if (iSong < 0)
		iSong=playlist.size()-1;

	Play(iSong);

}

/// \brief Start playing entry \e iSong in current playlist
///	\param iSong Song in playlist
void CPlayListPlayer::Play(int iSong)
{
	if (m_iCurrentPlayList==PLAYLIST_NONE)
		return;

	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	if (iSong < 0) iSong=0;
	if (iSong >= playlist.size() ) iSong=playlist.size()-1;

	m_bChanged=true;
	int iPreviousSong=m_iCurrentSong;
	m_iCurrentSong=iSong;
	const CPlayList::CPlayListItem& item=playlist[m_iCurrentSong];

	if (!CUtil::IsShoutCast(item.GetFileName()))
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_PLAY_NEXT_PREV, 0, 0, GetCurrentPlaylist(), MAKELONG(m_iCurrentSong, iPreviousSong), (LPVOID)&item );
		m_gWindowManager.SendThreadMessage( msg );
	}
	if(!g_application.PlayFile(item.GetFileName()))
	{
		//	Count entries in current playlist
		//	that couldn't be played
		m_iEntriesNotFound++;
	}
}

/// \brief Change the current song in playlistplayer.
///	\param iSong Song in playlist
void CPlayListPlayer::SetCurrentSong(int iSong)
{
	if (iSong >= -1 && iSong < GetPlaylist(GetCurrentPlaylist()).size() )
		m_iCurrentSong=iSong;
}

/// \brief Returns to current song in active playlist.
///	\return Current song
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

/// \brief Returns the active playlist.
///	\return Active playlist \n
///	Return values can be: \n
///	- PLAYLIST_NONE \n No playlist active
///	- PLAYLIST_MUSIC \n Playlist from music playlist window 
///	- PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
///	- PLAYLIST_VIDEO \n Playlist from music playlist window 
///	- PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
int CPlayListPlayer::GetCurrentPlaylist()
{
	return m_iCurrentPlayList;
}

/// \brief Set active playlist.
///	\param iPlayList Playlist to set active \n
///	Values can be: \n
///	- PLAYLIST_NONE \n No playlist active
///	- PLAYLIST_MUSIC \n Playlist from music playlist window 
///	- PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
///	- PLAYLIST_VIDEO \n Playlist from music playlist window 
///	- PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
void CPlayListPlayer::SetCurrentPlaylist( int iPlayList )
{
	if (iPlayList == m_iCurrentPlayList)
		return;
	m_iCurrentPlayList=iPlayList;
	m_iEntriesNotFound=0;
	m_bChanged=true;
}

/// \brief Get the playlist object specified in \e nPlayList
///	\param nPlayList Values can be: \n
///	- PLAYLIST_MUSIC \n Playlist from music playlist window 
///	- PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
///	- PLAYLIST_VIDEO \n Playlist from music playlist window 
///	- PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
///	\return A reference to the CPlayList object.
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
		m_PlaylistEmpty.Clear();
		return m_PlaylistEmpty;
		break;
	}
}

/// \brief Removes any item from all playlists located on a removable share
///	\return Number of items removed from PLAYLIST_MUSIC and PLAYLIST_VIDEO
int CPlayListPlayer::RemoveDVDItems()
{
	int nRemovedM=m_PlaylistMusic.RemoveDVDItems();
	m_PlaylistMusicTemp.RemoveDVDItems();
	int nRemovedV=m_PlaylistVideo.RemoveDVDItems();
	m_PlaylistVideoTemp.RemoveDVDItems();

	return nRemovedM+nRemovedV;
}

/// \brief Resets the playlistplayer, but the active playlist stays the same.
void CPlayListPlayer::Reset()
{
  m_iCurrentSong=-1;
	m_iEntriesNotFound=0;
}

/// \brief Number of playlist entries of the active playlist couldn't be played.
int CPlayListPlayer::GetEntriesNotFound()
{
	return m_iEntriesNotFound;
}
