// MediaMonitor.h: interface for the CMediaMonitor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"
#include "StdString.h"
#include "IMDB.h"
#include "../FileItem.h"
#include "../FileSystem/Directory.h"
#include "../VideoDatabase.h"

#include <vector>

using namespace std;

class IMediaObserver
{
public:
	virtual void OnMediaUpdate(	INT nIndex, CStdString& strFilename,
								CStdString& strTitle, 
								CStdString& strPictureFilePath) = 0;
};

class CMediaMonitor : CThread
{
public:

	struct Movie
	{
		CStdString	strFilepath;
		DWORD		dwDate;
		WORD		wTime;
	};

	typedef std::vector<Movie> MOVIELIST;

	CMediaMonitor();
	virtual ~CMediaMonitor();

	void Create(IMediaObserver* aObserver);

private:
	void Process();
	void Scan(DIRECTORY::CDirectory& directory, CStdString& aPath);
	bool GetMovieInfo(CStdString& strFilepath, CIMDBMovie& aMovieRecord);
	bool GetMovieArt(CStdString& strIMDBNumber, CStdString& strPictureUrl, CStdString& strImagePath);

	static bool SortMoviesByDateAndTime(Movie aMovie1, Movie aMovie2);

	static void parse_Clean(CStdString& strFilename);
	static bool parse_Similar(CStdString& strFilepath1, CStdString& strFilepath2, int aPercentage);
	static int	parse_GetStart(char* szText);
	static int	parse_GetLength(char* szText, int start);
	IMediaObserver*	m_pObserver;
	MOVIELIST		m_movies;
	CVideoDatabase  m_database;
};

#endif // !defined(AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
