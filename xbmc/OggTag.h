//------------------------------
// COggTag in 2003 by Bobbin007
//------------------------------

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
		int GetDuration() { return m_nDuration; }
		CStdString GetGenre() { return m_strGenre; }

	protected:
		void ProcessVorbisComment(const char *pBuffer);

		int parseTagEntry(CStdString& strTagEntry);
		void SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue);
		CFile* m_file;

		CStdString m_strTitle;
		CStdString m_strArtist;
		CStdString m_strYear;
		CStdString m_strAlbum;
		int m_nTrackNum;
		int m_nBitrate;
		int m_nSamplesPerSec;
		int m_nChannels;
		__int64 m_nSamples;		// number of samples in file
		int m_nDuration;		// duration in frames (75th of a second)
		CStdString m_strGenre;

	};
};