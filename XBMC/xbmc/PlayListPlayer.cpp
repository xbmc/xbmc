
#include "stdafx.h"
#include "PlayListPlayer.h"
#include "application.h"
#include "util.h"

#define PLAYLIST_MUSIC_REPEAT						0x001
#define PLAYLIST_MUSIC_REPEAT_ONE				0x002
#define PLAYLIST_MUSIC_TEMP_REPEAT			0x004
#define PLAYLIST_MUSIC_TEMP_REPEAT_ONE	0x008
#define PLAYLIST_VIDEO_REPEAT						0x010
#define PLAYLIST_VIDEO_REPEAT_ONE				0x020
#define PLAYLIST_VIDEO_TEMP_REPEAT			0x040
#define PLAYLIST_VIDEO_TEMP_REPEAT_ONE	0x080
#define PLAYLIST_MUSIC_SHUFFLE					0x100
#define PLAYLIST_MUSIC_TEMP_SHUFFLE			0x200
#define PLAYLIST_VIDEO_SHUFFLE					0x400
#define PLAYLIST_VIDEO_TEMP_SHUFFLE			0x800

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
	m_iCurrentSong=-1;
	m_bChanged=false;
	m_iEntriesNotFound=0;
	m_iCurrentPlayList=PLAYLIST_NONE;
	m_iOptions=0;
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
			if (m_iCurrentPlayList!=PLAYLIST_NONE)
			{
				CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
				m_gWindowManager.SendThreadMessage(msg);
				Reset();
				m_iCurrentPlayList=PLAYLIST_NONE;
			}
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

	if (ShuffledPlay(m_iCurrentPlayList))
	{
		while (iSong==m_iCurrentSong)
			iSong=NextShuffleItem();
	}
	else
	{
		if (!RepeatedOne(m_iCurrentPlayList))
			iSong++;
	}

	if (iSong >= playlist.size() )
  {
		iSong=0;

		if (!Repeated(m_iCurrentPlayList))
		{
			CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
			m_gWindowManager.SendThreadMessage(msg);
			Reset();
			m_iCurrentPlayList=PLAYLIST_NONE;
			return;
		}
  }

	if (bAutoPlay)
	{
		const CPlayList::CPlayListItem& item = playlist[iSong];
		if ( item.IsShoutCast() )
		{
			return;
		}
	}
	Play(iSong, bAutoPlay);
}

/// \brief Play previous entry in current playlist
void CPlayListPlayer::PlayPrevious()
{
	if (m_iCurrentPlayList==PLAYLIST_NONE)
		return;

	CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
	if (playlist.size() <= 0) return;
	int iSong=m_iCurrentSong;

	if (ShuffledPlay(m_iCurrentPlayList))
	{
		while (iSong==m_iCurrentSong)
			iSong=NextShuffleItem();
	}
	else
	{
		if (!RepeatedOne(m_iCurrentPlayList))
			iSong--;
	}

	if (iSong < 0)
		iSong=playlist.size()-1;

	Play(iSong);

}

/// \brief Start playing entry \e iSong in current playlist
///	\param iSong Song in playlist
void CPlayListPlayer::Play(int iSong, bool bAutoPlay)
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

	if (!item.IsShoutCast())
	{
		if (iPreviousSong<0)
		{
			CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STARTED, 0, 0, m_iCurrentPlayList, m_iCurrentSong, (LPVOID)&item);
			m_gWindowManager.SendThreadMessage( msg );
		}
		else
		{
			CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, m_iCurrentPlayList, MAKELONG(m_iCurrentSong, iPreviousSong), (LPVOID)&item);
			m_gWindowManager.SendThreadMessage(msg);
		}
	}

	if(!g_application.PlayFile(item, bAutoPlay))
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

/// \brief Repeat a playlist
///	\param iPlaylist Playlist to enable
///	\param bYesNo To Enable Repeat set \e true
void CPlayListPlayer::Repeat(int iPlaylist, bool bYesNo)
{
	int iOption=-1;
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		iOption=PLAYLIST_MUSIC_REPEAT;
		break;
	case PLAYLIST_MUSIC_TEMP:
		iOption=PLAYLIST_MUSIC_TEMP_REPEAT;
		break;
	case PLAYLIST_VIDEO:
		iOption=PLAYLIST_VIDEO_REPEAT;
		break;
	case PLAYLIST_VIDEO_TEMP:
		iOption=PLAYLIST_VIDEO_TEMP_REPEAT;
		break;
	default:
		break;
	}

	if (iOption<0)
		return;

	if (bYesNo)
		m_iOptions|=iOption;
	else
		m_iOptions&=~iOption;
}

/// \brief Returns \e true if iPlaylist is repeated
///	\param iPlaylist Playlist to be asked
bool CPlayListPlayer::Repeated(int iPlaylist)
{
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		if ((m_iOptions & PLAYLIST_MUSIC_REPEAT))
				return true;
		break;
	case PLAYLIST_MUSIC_TEMP:
		if ((m_iOptions & PLAYLIST_MUSIC_TEMP_REPEAT))
				return true;
		break;
	case PLAYLIST_VIDEO:
		if ((m_iOptions & PLAYLIST_VIDEO_REPEAT))
				return true;
		break;
	case PLAYLIST_VIDEO_TEMP:
		if ((m_iOptions & PLAYLIST_VIDEO_TEMP_REPEAT))
				return true;
		break;
	default:
		break;
	}

	return false;
}

/// \brief Repeat one song in a playlist
///	\param iPlaylist Playlist to Repeat
///	\param bYesNo To Enable Repeat one set \e true
void CPlayListPlayer::RepeatOne(int iPlaylist, bool bYesNo)
{
	int iOption=-1;
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		iOption=PLAYLIST_MUSIC_REPEAT_ONE;
		break;
	case PLAYLIST_MUSIC_TEMP:
		iOption=PLAYLIST_MUSIC_TEMP_REPEAT_ONE;
		break;
	case PLAYLIST_VIDEO:
		iOption=PLAYLIST_VIDEO_REPEAT_ONE;
		break;
	case PLAYLIST_VIDEO_TEMP:
		iOption=PLAYLIST_VIDEO_TEMP_REPEAT_ONE;
		break;
	default:
		break;
	}

	if (iOption<0)
		return;

	if (bYesNo)
		m_iOptions|=iOption;
	else
		m_iOptions&=~iOption;
}

/// \brief Returns \e true if iPlaylist repeats one song 
///	\param iPlaylist Playlist to be asked
bool CPlayListPlayer::RepeatedOne(int iPlaylist)
{
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		if ((m_iOptions & PLAYLIST_MUSIC_REPEAT_ONE))
				return true;
		break;
	case PLAYLIST_MUSIC_TEMP:
		if ((m_iOptions & PLAYLIST_MUSIC_TEMP_REPEAT_ONE))
				return true;
		break;
	case PLAYLIST_VIDEO:
		if ((m_iOptions & PLAYLIST_VIDEO_REPEAT_ONE))
				return true;
		break;
	case PLAYLIST_VIDEO_TEMP:
		if ((m_iOptions & PLAYLIST_VIDEO_TEMP_REPEAT_ONE))
				return true;
		break;
	default:
		break;
	}

	return false;
}

/// \brief Shuffle play the current playlist
///	\param bYesNo To Enable shuffle play, set to \e true
void CPlayListPlayer::ShufflePlay(int iPlaylist, bool bYesNo)
{
	int iOption=-1;
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		iOption=PLAYLIST_MUSIC_SHUFFLE;
		break;
	case PLAYLIST_MUSIC_TEMP:
		iOption=PLAYLIST_MUSIC_TEMP_SHUFFLE;
		break;
	case PLAYLIST_VIDEO:
		iOption=PLAYLIST_VIDEO_SHUFFLE;
		break;
	case PLAYLIST_VIDEO_TEMP:
		iOption=PLAYLIST_VIDEO_TEMP_SHUFFLE;
		break;
	default:
		break;
	}

	if (iOption<0)
		return;

	if (bYesNo)
		m_iOptions|=iOption;
	else
		m_iOptions&=~iOption;
}

/// \brief Shuffle play the current playlist
///	\param bYesNo To Enable shuffle play, set to \e true
bool CPlayListPlayer::ShuffledPlay(int iPlaylist)
{
	switch (iPlaylist)
	{
	case PLAYLIST_MUSIC:
		if ((m_iOptions & PLAYLIST_MUSIC_SHUFFLE))
				return true;
		break;
	case PLAYLIST_MUSIC_TEMP:
		if ((m_iOptions & PLAYLIST_MUSIC_TEMP_SHUFFLE))
				return true;
		break;
	case PLAYLIST_VIDEO:
		if ((m_iOptions & PLAYLIST_VIDEO_SHUFFLE))
				return true;
		break;
	case PLAYLIST_VIDEO_TEMP:
		if ((m_iOptions & PLAYLIST_VIDEO_TEMP_SHUFFLE))
				return true;
		break;
	default:
		break;
	}

	return false;
}

int CPlayListPlayer::NextShuffleItem()
{
	srand( timeGetTime() );

	CPlayList& playlist=GetPlaylist(GetCurrentPlaylist());
	int nItemCount = playlist.size();
	if (nItemCount<=0)
		return -1;

	return rand() % nItemCount;
}
