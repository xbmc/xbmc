#pragma once
#include "idirectory.h"

extern "C" {
	#include "../lib/libXDAAP/client.h"
	#include "../lib/libXDAAP/private.h"
}

using namespace DIRECTORY;

namespace DIRECTORY
{
	class CDAAPDirectory :

    public IDirectory
	{
public:
		CDAAPDirectory(void);
		virtual ~CDAAPDirectory(void);
		virtual bool  GetDirectory(const CStdString& strPath,CFileItemList &items);
		virtual void  CloseDAAP(void);

private:
		void free_albums(albumPTR *alb);
		void free_artists();
		void AddToArtistAlbum(char *artist_s, char *album_s);
		int GetCurrLevel(CStdString strPath);

		DAAP_ClientHost_DatabaseItem *m_currentSongItems;
		int m_currentSongItemCount;

		DAAP_SClient *m_thisClient;
		DAAP_SClientHost *m_thisHost;
		int m_currLevel;

		artistPTR *m_artisthead;
		CStdString m_selectedPlaylist;
		CStdString m_selectedArtist;
		CStdString m_selectedAlbum;
	};
}