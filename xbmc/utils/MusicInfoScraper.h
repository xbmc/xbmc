#pragma once
#include "MusicAlbumInfo.h"

using namespace MUSIC_GRABBER;

namespace MUSIC_GRABBER
{
	class CMusicInfoScraper
	{
	public:
		CMusicInfoScraper(void);
		virtual ~CMusicInfoScraper(void);
		bool							FindAlbuminfo(const CStdString& strAlbum);
		int								GetAlbumCount() const;
		CMusicAlbumInfo&  GetAlbum(int iAlbum) ;
	protected:
		vector<CMusicAlbumInfo> m_vecAlbums;
	};

};