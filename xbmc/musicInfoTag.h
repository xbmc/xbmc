#pragma once

class CSong;
class CAlbum;

#include "utils/archive.h"

namespace MUSIC_INFO
{

	class CMusicInfoTag : public ISerializable
	{
	public:
		CMusicInfoTag(void);
		CMusicInfoTag(const CMusicInfoTag& tag);
		virtual ~CMusicInfoTag();
		const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
		bool							Load(const CStdString& strFileName);
		void							Save(const CStdString& strFileName);
		bool							Loaded() const;
		const CStdString& GetTitle() const;
		const CStdString& GetURL() const;
		const CStdString& GetArtist() const;
		const CStdString& GetAlbum() const;
		const CStdString& GetGenre() const;
		int								GetTrackNumber() const;
		int								GetDuration() const;
		void							GetReleaseDate(SYSTEMTIME& dateTime) const;
		CStdString				GetYear() const;

		void 							SetURL(const CStdString& strURL) ;
		void 							SetTitle(const CStdString& strTitle) ;
		void 							SetArtist(const CStdString& strArtist) ;
		void 							SetAlbum(const CStdString& strAlbum) ;
		void 							SetGenre(const CStdString& strGenre) ;
		void							SetReleaseDate(SYSTEMTIME& dateTime);
		void							SetTrackNumber(int iTrack);
		void							SetDuration(int iSec);
		void							SetLoaded(bool bOnOff=true);
		void							SetAlbum(const CAlbum& album);
		void							SetSong(const CSong& song);

		virtual void			Serialize(CArchive& ar);

		void							Clear();
	protected:
		CStdString	m_strURL;
		CStdString	m_strTitle;
		CStdString	m_strArtist;
		CStdString	m_strAlbum;
		CStdString	m_strGenre;
		int					m_iDuration;
		int					m_iTrack;
		bool			  m_bLoaded;
		SYSTEMTIME  m_dwReleaseDate;
	};
};