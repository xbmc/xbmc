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

#define RECENT_MOVIES	3

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

	enum CommandType {Refresh,Update};

	struct Command
	{
		CommandType rCommand;
		CStdString  strParam1;
		CStdString  strParam2;
		INT			nParam1;
	};

	void Create(IMediaObserver* aObserver);
	void QueueCommand(CMediaMonitor::Command& aCommand);

protected:

	vector<Command> m_commands;
	typedef vector<Command> ::iterator COMMANDITERATOR;
	void DispatchNextCommand();

private:
	void Process();
	void InitializeObserver();
	void Scan();
	void Scan(DIRECTORY::CDirectory& directory, CStdString& aPath);
	void UpdateTitle(INT nIndex, CStdString& strTitle, CStdString& strFilepath);
	bool GetMovieInfo(CStdString& strFilepath, CIMDBMovie& aMovieRecord);
	bool imdb_GetMovieInfo(CStdString& strTitle, CIMDBMovie& aMovieRecord);
	bool imdb_GetMovieArt(CStdString& strIMDBNumber, CStdString& strPictureUrl, CStdString& strImagePath);

	static bool SortMoviesByDateAndTime(Movie aMovie1, Movie aMovie2);

	static void parse_Clean(CStdString& strFilename);
	static bool parse_Similar(CStdString& strFilepath1, CStdString& strFilepath2, int aPercentage);
	static int	parse_GetStart(char* szText);
	static int	parse_GetLength(char* szText, int start);
	static long parse_AggregateValue(CStdString& strFilepath);

	IMediaObserver*		m_pObserver;
	MOVIELIST			m_movies;
	CVideoDatabase		m_database;
	CRITICAL_SECTION	m_critical_section;
	HANDLE				m_hCommand;
};

#endif // !defined(AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
