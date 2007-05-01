// MediaMonitor.h: interface for the CMediaMonitor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"
#include "IMDB.h"
#include "../FileSystem/IDirectory.h"
#include "../VideoDatabase.h"
#include "../Settings.h"

#define RECENT_MOVIES 3

class IMediaObserver
{
public:
  virtual void OnMediaUpdate( INT nIndex, CStdString& strFilename,
                              CStdString& strTitle,
                              CStdString& strPictureFilePath) = 0;
};

class CMediaMonitor : CThread
{
public:

  struct Movie
  {
    CStdString strFilepath;
    DWORD dwDate;
    WORD wTime;
  };

  typedef std::vector<Movie> MOVIELIST;
  typedef std::vector<Movie> ::iterator MOVIELISTITERATOR;

  virtual ~CMediaMonitor();

  static CMediaMonitor *GetInstance(IMediaObserver* aObserver = NULL);
  static void RemoveInstance();

  enum CommandType {Seed, Refresh, Update};

  struct Command
  {
    CommandType rCommand;
    CStdString strParam1;
    CStdString strParam2;
    INT nParam1;
  };

  void Create(IMediaObserver* aObserver);
  void QueueCommand(CMediaMonitor::Command& aCommand);
  bool IsBusy();
  void SetObserver(IMediaObserver* observer);

protected:

  vector<Command> m_commands;
  typedef vector<Command> ::iterator COMMANDITERATOR;
  void DispatchNextCommand();

private:
  static CMediaMonitor *monitor;
  CMediaMonitor();
  void OnStartup();
  void Process();
  void InitializeObserver();
  void Scan(bool bUpdateAllMovies = false);
  void Scan(DIRECTORY::IDirectory& directory, CStdString& aPath, MOVIELIST& movies);
  void GetSharedMovies(VECSHARES& vecShares, MOVIELIST& movies);
  void FilterDuplicates(MOVIELIST& movies);

  void UpdateTitle(INT nIndex, CStdString& strTitle, CStdString& strFilepath);
  void UpdateObserver(Movie& aMovie, INT nIndex, bool bForceUpdate, bool bUpdateObserver);
  bool GetMovieInfo(CStdString& strFilepath, CIMDBMovie& aMovieRecord, bool bRefresh);
  bool imdb_GetMovieInfo(CStdString& strTitle, CIMDBMovie& aMovieRecord, const CStdString& strScraper);
  bool imdb_GetMovieArt(CStdString& strIMDBNumber, CStdString& strPictureUrl, CStdString& strImagePath);

  static bool SortMoviesByDateAndTime(Movie aMovie1, Movie aMovie2);

  static void parse_Clean(CStdString& strFilename);
  static bool parse_Similar(CStdString& strFilepath1, CStdString& strFilepath2, int aPercentage);
  static int parse_GetStart(char* szText);
  static int parse_GetLength(char* szText, int start);
  static long parse_AggregateValue(CStdString& strFilepath);

  IMediaObserver* m_pObserver;
  CVideoDatabase m_database;
  CRITICAL_SECTION m_critical_section;
  HANDLE m_hCommand;
  bool m_bBusy;
};

#endif // !defined(AFX_MediaMonitor_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
