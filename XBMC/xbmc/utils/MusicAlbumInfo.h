#pragma once
#include "stdstring.h"
#include "musicsong.h"
#include <vector>
using namespace std;

using namespace MUSIC_GRABBER;
namespace MUSIC_GRABBER
{
	class CMusicAlbumInfo
	{
	public:
		CMusicAlbumInfo(void);
		CMusicAlbumInfo(const CStdString& strAlbumInfo, const CStdString& strAlbumURL);
		virtual ~CMusicAlbumInfo(void);
		void							Save(CStdString& strFileName);
		bool							Load(CStdString& strFileName);
		bool							Loaded() const;
		void							SetLoaded(bool bOnOff);

		const CStdString& GetArtist() const;
		const CStdString& GetTitle() const;
		const CStdString& GetTitle2() const;
		const CStdString& GetDateOfRelease() const;
		const CStdString& GetGenre() const;
		const CStdString& GetTones() const;
		const CStdString& GetStyles() const;
		const CStdString& GetReview() const;
		const CStdString& GetImageURL() const;
		int								GetRating() const;
		int								GetNumberOfSongs() const;
		const CMusicSong& GetSong(int iSong);
		const CStdString& GetAlbumURL() const;
		bool							Load();
		bool							Parse(const CStdString& strHTML);
	protected:
		CStdString		m_strArtist;
		CStdString		m_strTitle;
		CStdString		m_strTitle2;
		CStdString		m_strDateOfRelease;
		CStdString		m_strGenre;
		CStdString		m_strTones;
		CStdString		m_strStyles;
		CStdString    m_strReview;
		CStdString	  m_strImageURL;
		CStdString    m_strAlbumURL;
		int						m_iRating;
		bool					m_bLoaded;
		vector<CMusicSong> m_vecSongs;
	};
};