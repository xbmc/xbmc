#pragma once

#include "StdString.h"
#include "playlist.h"
#include "IMsgTargetCallback.h"
#include "GUIMessage.h"
#include <vector>

#define PLAYLIST_MUSIC			0
#define PLAYLIST_MUSIC_TEMP	1
#define PLAYLIST_VIDEO			2
#define PLAYLIST_VIDEO_TEMP	3

using namespace std;
using namespace PLAYLIST;

namespace PLAYLIST
{
	class CPlayListPlayer : public IMsgTargetCallback
	{
	public:
		CPlayListPlayer(void);
		virtual ~CPlayListPlayer(void);
		virtual bool OnMessage(CGUIMessage &message);
		void			PlayNext(bool bAutoPlay=false);
		void			PlayPrevious();
		void			Play(int iSong);
		int				GetCurrentSong() const;
		void					SetCurrentSong(int iSong);
		bool			HasChanged();
		void			SetCurrentPlaylist( int iPlayList );
		int					GetCurrentPlaylist();
		CPlayList&	GetPlaylist( int nPlayList);
		int					RemoveDVDItems();
		virtual void Shuffle();
	protected:
		bool				m_bChanged;
		int					m_iCurrentSong;
		int					m_iCurrentPlayList;
		CPlayList		m_PlaylistMusic;
		CPlayList		m_PlaylistMusicTemp;
		CPlayList		m_PlaylistVideo;
		CPlayList		m_PlaylistVideoTemp;
	};

};
extern CPlayListPlayer g_playlistPlayer;

