#pragma once

#include "musicdatabase.h"

#define MAX_PATH_SIZE 1024
#define MAX_CUE_TRACKS 99

class CCueDocument
{
	struct CUEITEM
	{
		CStdString	strArtist;
		CStdString	strTitle;
		int		iTrackNumber;
		int		iStartTime;
		int		iEndTime;
	};

	public:
	CCueDocument(void);
	~CCueDocument(void);
	// USED
	bool	Parse(const CStdString &strFile);
	void	GetSongs(VECSONGS &songs);
	CStdString	GetMediaPath();
	CStdString	GetMediaTitle();

private:

	// USED for file access
	CFile m_file;
	char m_szBuffer[1024];

	// Member variables
	CStdString m_strArtist;		// album artist
	CStdString m_strAlbum;		// album title
	CStdString m_strFilePath;	// path of underlying media
	int m_iTrack;			// current track
	int m_iTotalTracks;		// total tracks

	// cuetrack array
	CUEITEM m_Track[MAX_CUE_TRACKS];

	bool				ReadNextLine(CStdString &strLine);
	bool				ExtractQuoteInfo(CStdString &szData, const char *szLine);
	int					ExtractTimeFromString(const char *szData);
	int					ExtractNumericInfo(const char *szData);
	bool				ResolvePath(CStdString &strPath, const CStdString &strBase);
};
