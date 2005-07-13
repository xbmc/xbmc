// MediaMonitor.cpp: implementation of the CMediaMonitor class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "MediaMonitor.h"
#include "../Picture.h"
#include "../Util.h"
#include "../FileSystem/VirtualDirectory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CMediaMonitor::SortMoviesByDateAndTime(Movie aMovie1, Movie aMovie2)
{
  if (aMovie1.dwDate != aMovie2.dwDate)
  {
    return (aMovie1.dwDate < aMovie2.dwDate );
  }

  return (aMovie1.wTime < aMovie2.wTime );
}

CMediaMonitor::CMediaMonitor() : CThread()
{
  m_pObserver = NULL;
  m_bBusy = false;
}

CMediaMonitor::~CMediaMonitor()
{
  CloseHandle(m_hCommand);
  DeleteCriticalSection(&m_critical_section);
}

void CMediaMonitor::Create(IMediaObserver* aObserver)
{
  InitializeCriticalSection(&m_critical_section);

  m_bStop = false;
  m_pObserver = aObserver;
  m_hCommand = CreateEvent(NULL, FALSE, FALSE, NULL);

  CThread::Create(false);
}

void CMediaMonitor::OnStartup()
{
  SetPriority( THREAD_PRIORITY_LOWEST );
}

void CMediaMonitor::Process()
{
  InitializeObserver();

  CMediaMonitor::Command command;
  command.rCommand = CMediaMonitor::CommandType::Refresh;
  QueueCommand(command);

  while (!m_bStop)
  {
    switch ( WaitForSingleObject(m_hCommand, INFINITE) )
    {
    case WAIT_OBJECT_0:
      m_bBusy = true;
      DispatchNextCommand();
      m_bBusy = false;
      break;
    }
  }
}

/// This method queues a command for processing by the thread when it is next available
void CMediaMonitor::QueueCommand(CMediaMonitor::Command& aCommand)
{
  EnterCriticalSection(&m_critical_section);

  m_commands.push_back(aCommand);

  LeaveCriticalSection(&m_critical_section);

  SetEvent(m_hCommand);
}

bool CMediaMonitor::IsBusy()
{
  return m_bBusy;
}

/// This method checks the list of pending commands and executes the next command
void CMediaMonitor::DispatchNextCommand()
{
  if (m_commands.size() <= 0)
  {
    return ;
  }

  EnterCriticalSection(&m_critical_section);

  COMMANDITERATOR it = m_commands.begin();
  CMediaMonitor::Command command = *it;
  m_commands.erase(it);

  LeaveCriticalSection(&m_critical_section);

  switch (command.rCommand)
  {
  case CommandType::Seed:
    {
      // update all movie information
      CLog::Log(LOGINFO, "Seeding database... started.");
      Scan(true);
      CLog::Log(LOGINFO, "Seeding database... completed.");
      break;
    }

  case CommandType::Refresh:
    {
      // get information for the latest 3 movies
      CLog::Log(LOGINFO, "Refreshing latest movies... started.");
      Scan(false);
      CLog::Log(LOGINFO, "Refreshing latest movies... completed.");
      break;
    }

  case CommandType::Update:
    {
      // update the information for a specific movie
      CLog::Log(LOGINFO, "Updating movie... started.");
      UpdateTitle(command.nParam1, command.strParam1, command.strParam2);
      CLog::Log(LOGINFO, "Updating movie... completed.");
      break;
    }
  }
}


/// This method scans all remote shares and determines the latest 3 movies
/// If bUpdateAllMovies is true then IMDB is queried for each and every movie found
//  otherwise queries are limited to the 3 latest movies.
void CMediaMonitor::Scan(bool bUpdateAllMovies)
{
  MOVIELIST movies;
  GetSharedMovies(g_settings.m_vecMyVideoShares, movies);

  // sort files by creation date
  sort(movies.begin(), movies.end(), CMediaMonitor::SortMoviesByDateAndTime );

  // remove duplicates
  FilterDuplicates(movies);

  // Update movies if necessary
  if (bUpdateAllMovies)
  {
    // query imdb for new info
    for (int iMovie = 0; iMovie < (int)movies.size(); iMovie++)
    {
      UpdateObserver(movies[iMovie], NULL, 0, false);
      Sleep(50);
    }
  }

  // Select the 3 newest files (remove similar filenames from consideration):
  int iLatestMovie = (int) movies.size() - 1;
  int iOlderMovie = iLatestMovie - RECENT_MOVIES;
  for (int iMovie = iLatestMovie; (iMovie >= 0) && (iMovie > iOlderMovie); iMovie--)
  {
    UpdateObserver(movies[iMovie], m_pObserver, iLatestMovie - iMovie, false);
  }

  movies.clear();
}


void CMediaMonitor::GetSharedMovies(VECSHARES& vecShares, MOVIELIST& movies)
{
  DIRECTORY::CVirtualDirectory directory;
  directory.SetShares(vecShares);
  directory.SetMask(g_stSettings.m_szMyVideoExtensions);

  CStdString path;
  CFileItemList items;
  if (directory.GetDirectory(path, items))
  {
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem* pItem = items[i];
      if (pItem->m_iDriveType == SHARE_TYPE_REMOTE)
      {
        Scan(directory, pItem->m_strPath, movies);
      }
    }
  }
}

void CMediaMonitor::FilterDuplicates(MOVIELIST& movies)
{
  MOVIELISTITERATOR current = movies.begin();
  MOVIELISTITERATOR previous = current;

  while (current != movies.end())
  {
    if (previous != current)
    {
      // ensure at least less than 75% similar compared to the previous selection
      if (parse_Similar( (*current).strFilepath, (*previous).strFilepath, 75))
      {
        // items are similar, find out which one is more likely to be CD1
        long valueA = parse_AggregateValue((*current).strFilepath);
        long valueB = parse_AggregateValue((*previous).strFilepath);

        if (valueA < valueB)
        {
          *previous = *current;
        }

        current = movies.erase(current);
        continue;
      }
    }

    previous = current;
    current++;
  }
}

/// This method gets information for the specified movie and notifies the specified
/// observer.
void CMediaMonitor::UpdateObserver(Movie& aMovie, IMediaObserver* pObserver, INT nIndex, bool bForceUpdate)
{
  CStdString strImagePath;
  CIMDBMovie details;

  if ( GetMovieInfo(aMovie.strFilepath, details, bForceUpdate) )
  {
    if (!details.m_strPictureURL.IsEmpty())
    {
      imdb_GetMovieArt(details.m_strIMDBNumber, details.m_strPictureURL, strImagePath);
    }
  }
  else
  {
    details.m_strTitle = CUtil::GetFileName(aMovie.strFilepath);
  }

  if (pObserver)
  {
    g_graphicsContext.Lock();
    pObserver->OnMediaUpdate(nIndex, aMovie.strFilepath, details.m_strTitle, strImagePath);
    g_graphicsContext.Unlock();
  }
}

/// This method looks for information in the localdatabase, checking IMDB for new information
/// if none is available in the localdatabase or bRefresh is true.
bool CMediaMonitor::GetMovieInfo(CStdString& strFilepath, CIMDBMovie& aMovieRecord, bool bRefresh)
{
  m_database.Open();

  // Return the record stored in local database
  if (m_database.HasMovieInfo(strFilepath))
  {
    if (bRefresh)
    {
      m_database.DeleteMovie(strFilepath);
    }
    else
    {
      m_database.GetMovieInfo(strFilepath, aMovieRecord);
      m_database.Close();
      return true;
    }
  }

  // Clean up filename in an attempt to get an idea of the movie name
  CStdString strName = CUtil::GetFileName(strFilepath);
  parse_Clean(strName);

  if (strName.IsEmpty())
  {
    strName = CUtil::GetFileName(strFilepath);
  }

  CStdString debug;
  debug.Format("New filepath: %s\nQuerying: %s", strFilepath.c_str(), strName.c_str());
  CLog::Log(LOGDEBUG, debug);

  // Query IMDB and populate the record
  if (imdb_GetMovieInfo(strName, aMovieRecord))
  {
    // Store the record in the internal database
    CStdString strCDLabel;
    if (m_database.AddMovie(strFilepath, strCDLabel, false))
    {
      m_database.SetMovieInfo(strFilepath, aMovieRecord);
    }

    m_database.Close();
    return true;
  }

  // We failed to find any information
  m_database.Close();
  return false;
}

/// This method queries imdb for movie information associated with a title
bool CMediaMonitor::imdb_GetMovieInfo(CStdString& strTitle, CIMDBMovie& aMovieRecord)
{
  CIMDB imdb;
  IMDB_MOVIELIST results;

  if (imdb.FindMovie(strTitle, results) )
  {
    if (results.size() > 0)
    {
      if (! imdb.GetDetails(results[0], aMovieRecord) )
      {
        aMovieRecord.m_strTitle = results[0].m_strTitle;
      }
      return true;
    }
  }

  return false;
}

/// This method queries imdb for movie poster art associated with an imdb number
bool CMediaMonitor::imdb_GetMovieArt(CStdString& strIMDBNumber, CStdString& strPictureUrl, CStdString& strImagePath)
{
  CStdString strThum;
  CUtil::GetVideoThumbnail(strIMDBNumber, strThum);

  if (CFile::Exists(strThum.c_str()))
  {
    strImagePath = strThum;
    return true;
  }

  CStdString strExtension;
  CUtil::GetExtension(strPictureUrl, strExtension);

  if (strExtension.IsEmpty())
  {
    return false;
  }

  CStdString strTemp;
  strTemp.Format("Z:\\ram_temp%s", strExtension.c_str());
  ::DeleteFile(strTemp.c_str());

  CHTTP http;
  http.Download(strPictureUrl, strTemp);

  try
  {
    CPicture picture;
    picture.Convert(strTemp, strThum);
  }
  catch (...)
  {
    ::DeleteFile(strThum.c_str());
  }

  ::DeleteFile(strTemp.c_str());

  if (CFile::Exists(strThum.c_str()))
  {
    strImagePath = strThum;
    return true;
  }

  return false;
}

/// This method queries forcibly updates an existing movies information by querying
/// imdb with the specified title
void CMediaMonitor::UpdateTitle(INT nIndex, CStdString& strTitle, CStdString& strFilepath)
{
  m_database.Open();

  CStdString strImagePath;
  CIMDBMovie details;
  bool bRecord = false;

  // Get the record stored in local database
  if (m_database.HasMovieInfo(strFilepath))
  {
    m_database.DeleteMovieInfo(strFilepath);
  }

  // Query IMDB and populate the record
  if (imdb_GetMovieInfo(strTitle, details))
  {
    if (!details.m_strPictureURL.IsEmpty())
    {
      imdb_GetMovieArt(details.m_strIMDBNumber, details.m_strPictureURL, strImagePath);
    }
  }
  else
  {
    details.m_strTitle = strTitle;
  }

  // Update the record in the internal database
  m_database.SetMovieInfo(strFilepath, details);

  if (m_pObserver)
  {
    g_graphicsContext.Lock();
    m_pObserver->OnMediaUpdate(nIndex, strFilepath, details.m_strTitle, strImagePath);
    g_graphicsContext.Unlock();
  }

  m_database.Close();
}


/// This method queries the local database for the last movies added
void CMediaMonitor::InitializeObserver()
{
  m_database.Open();

  long lzMovieId[RECENT_MOVIES];
  int count = m_database.GetRecentMovies(lzMovieId, RECENT_MOVIES);

  m_database.Close();

  for (int i = 0;i < count;i++)
  {
    m_database.Open();

    VECMOVIESFILES paths;
    m_database.GetFiles(lzMovieId[i], paths);

    m_database.Close();

    if (paths.size() > 0)
    {
      Movie movie;
      movie.strFilepath = paths[0];
      UpdateObserver(movie, m_pObserver, i, false);
    }
  }
}


void CMediaMonitor::Scan(DIRECTORY::IDirectory& directory, CStdString& aPath, MOVIELIST& movies)
{
  // dispatch any pending commands first before scanning this directory!
  DispatchNextCommand();

  CFileItemList items;
  if (!directory.GetDirectory(aPath, items))
  {
    return ;
  }

  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem* pItem = items[i];

    if (pItem->m_bIsFolder)
    {
      Scan(directory, pItem->m_strPath, movies);
    }
    else
    {
      char* szFilename = (char*) pItem->GetLabel().c_str();
      char* szExtension = strrchr(szFilename, '.');
      char* szExpected = szFilename + (strlen(szFilename) - 4);

      // ensure file has a 3 letter extension that isn't .nfo
      if ((szExtension != NULL) && (szExpected == szExtension) && (strcmp(szExtension, ".nfo") != 0) )
      {
        Movie aMovie;
        aMovie.strFilepath = pItem->m_strPath;
        aMovie.dwDate = (( (pItem->m_stTime.wYear) << 16 ) |
                         ( (pItem->m_stTime.wMonth & 0x00FF) << 8 ) |
                         (pItem->m_stTime.wDay & 0x00FF) );
        aMovie.wTime = (( (pItem->m_stTime.wHour & 0x00FF) << 8 ) |
                        (pItem->m_stTime.wMinute & 0x00FF) );

        movies.push_back(aMovie);

        //OutputDebugString(pItem->GetLabel().c_str());
        //OutputDebugString("\n");
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////
// Filename parsing functions

const char seps[] = ",._- ";
const char* keywords[] = {"xvid", "divx", "hdtv", "pdtv", "dvdrip", "dvd", "ts", "scr", 0};

int CMediaMonitor::parse_GetStart(char* szText)
{
  char szCopy[1024];
  strcpy(szCopy, szText);

  unsigned short nLineLen = strlen(szCopy);
  unsigned short nHalfLine = nLineLen >> 1;

  char* token = strtok( szCopy, seps );
  char* find = NULL;

  while (token != NULL )
  {
    if ((token - szCopy) > nHalfLine)
    {
      token = NULL;
      break;
    }

    for (int i = 0;keywords[i] != NULL;i++)
    {
      if (strcmp(keywords[i], token) == 0)
      {
        find = token + strlen(token) + 1;
      }
    }


    token = strtok( NULL, seps );
  }

  int start = 0;
  if (find != NULL)
  {
    start = (int) (find - szCopy);
  }

  return start;
}


int CMediaMonitor::parse_GetLength(char* szText, int start)
{
  char szCopy[1024];
  strcpy(szCopy, &szText[start]);

  char* szExtension = strrchr(szCopy, '.');

  if (szExtension == NULL)
  {
    szExtension = (szCopy + strlen(szCopy));
  }

  char* token = strtok( szCopy, seps );
  bool exit = false;

  while ((token != NULL ) && (!exit) && (token < szExtension))
  {
    unsigned short nWordLen = strlen(token);
    unsigned short nHalfWord = nWordLen >> 1;
    unsigned short nCapital = 0;

    for (int i = 0;i < nWordLen;i++)
    {
      if ((token[i] >= 'A') && (token[i] <= 'Z'))
      {
        nCapital++;
      }
    }

    if ((nCapital >= nHalfWord) && (nWordLen > 3))
    {
      break;
    }

    for (int j = 0;keywords[j] != NULL;j++)
    {
      if (strcmp(token, keywords[j]) == 0)
      {
        exit = true;
        break;
      }
    }

    if (exit)
    {
      break;
    }
    else
    {
      token = strtok( NULL, seps );
    }
  }

  int delimiter = 0;
  if (token != NULL)
    delimiter = (int)(token - szCopy);
  if (delimiter == 0)
    delimiter = strlen(&szText[start]);

  return delimiter;
}

void CMediaMonitor::parse_Clean(CStdString& strFilename)
{
  char* szBuffer = (char*) strFilename.c_str();
  int start = parse_GetStart ( szBuffer );
  int length = parse_GetLength( szBuffer, start);

  char szCopy[1024];

  if (length < 0)
  {
    length = strlen(&szBuffer[start]);
  }

  char lc = 0;
  for (int i = start, i2 = 0; ((i < start + length) && (szBuffer[i] != 0)); i++)
  {
    char c = szBuffer[i];

    switch (c)
    {
    case '\r':
    case '\n':
    case ',':
    case '.':
    case '_':
    case '-':
    case ' ':
      c = ' ';
      if (c == lc)
      {
        continue;
      }
      break;
    }

    szCopy[i2++] = c;
    lc = c;
  }

  szCopy[i2] = 0;
  CStdString name = szCopy;
  strFilename = name.Trim();
}

bool CMediaMonitor::parse_Similar(CStdString& strFilepath1, CStdString& strFilepath2, int aPercentage)
{
  char* szText1 = CUtil::GetFileName(strFilepath1);
  char* szText2 = CUtil::GetFileName(strFilepath2);

  for (int i = 0, identical = 0; ((szText1[i] != 0) && (szText2[i] != 0)); i++)
    if (szText1[i] == szText2[i])
      identical++;

  int len1 = strlen(szText1);
  int len2 = strlen(szText2);

  int similarity = (identical * 100) / ((len1 <= len2) ? len1 : len2);

  return (similarity >= aPercentage);
}

long CMediaMonitor::parse_AggregateValue(CStdString& strFilepath)
{
  long count = 0;
  for (int i = 0; i < strFilepath.GetLength(); i++)
  {
    count += strFilepath[i];
  }
  return count;
}
