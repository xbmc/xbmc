#pragma once

#include "StdString.h"
#include "playlist.h"
#include <vector>

using namespace std;
using namespace PLAYLIST;

namespace PLAYLIST
{
	class CPlayListPlayer: public CPlayList
	{
	public:
		CPlayListPlayer(void);
		virtual ~CPlayListPlayer(void);
		void					PlayNext(bool bAutoPlay=false);
		void					PlayPrevious();
		void					Play(int iSong);
		int						GetCurrentSong() const;
		void					SetCurrentSong(int iSong);
		bool					HasChanged() ;
		virtual void Shuffle();
	protected:
		bool			m_bChanged;
		int				m_iCurrentSong;
	};

};
extern CPlayListPlayer g_playlistPlayer;

