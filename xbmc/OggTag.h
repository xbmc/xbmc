//------------------------------
// COggTag in 2003 by Bobbin007
//------------------------------
#include "FileSystem/file.h"

namespace MUSIC_INFO {

#pragma once

	class COggTag
	{
	public:
		COggTag(void);
		virtual ~COggTag(void);
		bool ReadTag(CFile* file);
		CStdString GetTitle() { return m_strTitle; }
		CStdString GetArtist() { return m_strArtist; }
		CStdString GetYear() { return m_strYear; }
		CStdString GetAlbum() { return m_strAlbum; }
		int GetTrackNum() { return m_nTrackNum; }
		CStdString GetGenre() { return m_strGenre; }

	protected:
		bool FindVobisTagHeader(void);

		UINT ReadLength(void);
		char* ReadString( int nLenght );
		bool ReadBit(void);

		int parseTagEntry(CStdString& strTagEntry);
		void SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue);

	private:
		CFile* m_file;

		CStdString m_strTitle;
		CStdString m_strArtist;
		CStdString m_strYear;
		CStdString m_strAlbum;
		int m_nTrackNum;
		CStdString m_strGenre;
	};
};