//
// C++ Interface: cguikaraokesongselector
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KARAOKESONGSELECTOR_H
#define KARAOKESONGSELECTOR_H

#include "Song.h"


class CGUITextLayout;

class CKaraokeSongSelector
{
	public:
		CKaraokeSongSelector();
		~CKaraokeSongSelector();

		//! This function is called to render the state/help string
		void Render();

		//! Those functions control the selection process
		void OnButtonNumeric( unsigned int code ); // 0x00 - 0x09
		void OnButtonSelect(); // Song is selected

		//! Load the database from disk (temporary)
		void loadDatabase();
		
	protected:
		void updateOnScreenMessage();
		
	private:
		//! The song selector is active
		bool				m_selectorActive;

		//! The song selector on-screen message
		CStdString 			m_selectorMessage;
		
		//! Time when the song selector will become inactive
		unsigned int		m_inactiveTime;
		
		//! Currently selected number
		unsigned int		m_selectedNumber;
		
		//! True if the number above did select some song and the info is in m_karaokeData
		bool				m_songSelected;
		
		//! Database stuff
		CSong				m_karaokeSong;
};

#endif
