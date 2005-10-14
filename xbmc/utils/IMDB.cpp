// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "IMDB.h"
#include "../util.h"
#include "HTMLUtil.h"
#include "../FileSystem/FileCurl.h"

using namespace HTML;

#pragma warning (disable:4018) 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIMDB::CIMDB()
{
}

CIMDB::CIMDB(const CStdString& strProxyServer, int iProxyPort)
    : m_http(strProxyServer, iProxyPort)
{
}

CIMDB::~CIMDB()
{
}

bool CIMDB::InternalFindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movielist)
{
  // load our dll if need be
  if (!m_dll.Load())
    return false;

  CIMDBUrl url;
  movielist.clear();

  CStdString strURL, strHTML;
  GetURL(strMovie, strURL);

  if (!m_http.Get(strURL, strHTML) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "IMDB: Unable to retrieve web site");
    return false;
  }
  
  char *szXML = new char[80000];  // should be enough for 500 matches (max returned by IMDb)
  if (!m_dll.IMDbGetSearchResults(szXML, strHTML.c_str(), m_http.m_redirectedURL.c_str()))
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  // ok, now parse the xml file
  TiXmlDocument doc;
  if (!doc.Parse(szXML))
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
    return false;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement *movie = docHandle.FirstChild( "results" ).FirstChild( "movie" ).Element();

  for ( movie; movie; movie = movie->NextSiblingElement() )
  {
    TiXmlNode * title = movie->FirstChild("title");
    TiXmlNode *link = movie->FirstChild("url");
    if (title && title->FirstChild() && link && link->FirstChild())
    {
      url.m_strTitle = title->FirstChild()->Value();
      url.m_strURL = link->FirstChild()->Value();
      movielist.push_back(url);
    }
  }
  return true;
}

bool CIMDB::GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue)
{
  TiXmlNode* pNode = pRootNode->FirstChild(strTag );
  if (!pNode) return false;
  pNode = pNode->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode->Value();
    return true;
  }
  return false;
}

bool CIMDB::InternalGetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails)
{
  // load our dll if need be
  if (!m_dll.Load())
    return false;

  CStdString strHTML, strPlotHTML;
  CStdString strURL = url.m_strURL;

  // fill in the defaults
  CStdString strLocNotAvail = g_localizeStrings.Get(416); // Not available
  movieDetails.m_strTitle = url.m_strTitle;
  movieDetails.m_strDirector = strLocNotAvail;
  movieDetails.m_strWritingCredits = strLocNotAvail;
  movieDetails.m_strGenre = strLocNotAvail;
  movieDetails.m_strTagLine = strLocNotAvail;
  movieDetails.m_strPlotOutline = strLocNotAvail;
  movieDetails.m_strPlot = strLocNotAvail;
  movieDetails.m_strPictureURL = "";
  movieDetails.m_strRuntime = strLocNotAvail;
  movieDetails.m_iYear = 0;
  movieDetails.m_fRating = 0.0;
  movieDetails.m_strVotes = strLocNotAvail;
  movieDetails.m_strCast = strLocNotAvail;
  movieDetails.m_iTop250 = 0;

  if (!m_http.Get(strURL, strHTML) || strHTML.size() == 0)
    return false;

  // grab our plot summary as well (if it's available)
  m_http.Get(strURL + "plotsummary", strPlotHTML);

  char *szBuffer = new char[strHTML.size() + 1];
  strcpy(szBuffer, strHTML.c_str());

  // get the IMDb number from our URL
  int idxUrlParams = strURL.Find('?');
  if (idxUrlParams != -1)
  {
    strURL = strURL.Left(idxUrlParams);
  }
  char szURL[1024];
  strcpy(szURL, strURL.c_str());
  if (CUtil::HasSlashAtEnd(strURL)) szURL[ strURL.size() - 1 ] = 0;
  int ipos = strlen(szURL) - 1;
  while (szURL[ipos] != '/' && ipos > 0) ipos--;

  movieDetails.m_strIMDBNumber = &szURL[ipos + 1];
  CLog::Log(LOGINFO, "imdb number:%s\n", movieDetails.m_strIMDBNumber.c_str());

  // now grab our details using the dll
  char szXML[50000];
  if (!m_dll.IMDbGetDetails(szXML, strHTML.c_str(), strPlotHTML.c_str()))
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  // save the xml file for later reading...
  CFile file;
  CStdString strXMLFile;
  strXMLFile.Format("%s\\imdb\\%s.xml", g_stSettings.m_szAlbumDirectory, movieDetails.m_strIMDBNumber.c_str());
  if (file.OpenForWrite(strXMLFile))
  {
    file.Write(szXML, strlen(szXML));
    file.Close();
  }

  // ok, now parse the xml file
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  if (!doc.Parse(szXML))
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
    return false;
  }

  bool ret = ParseDetails(doc, movieDetails);
  TiXmlBase::SetCondenseWhiteSpace(true);

  return ret;
}

bool CIMDB::ParseDetails(TiXmlDocument &doc, CIMDBMovie &movieDetails)
{
  // fill in the defaults
  CStdString strLocNotAvail = g_localizeStrings.Get(416); // Not available
  movieDetails.m_strDirector = strLocNotAvail;
  movieDetails.m_strWritingCredits = strLocNotAvail;
  movieDetails.m_strGenre = strLocNotAvail;
  movieDetails.m_strTagLine = strLocNotAvail;
  movieDetails.m_strPlotOutline = strLocNotAvail;
  movieDetails.m_strPlot = strLocNotAvail;
  movieDetails.m_strPictureURL = "";
  movieDetails.m_strVotes = strLocNotAvail;
  movieDetails.m_strCast = strLocNotAvail;
  movieDetails.m_iTop250 = 0;

  TiXmlNode *details = doc.FirstChild( "details" );

  if (!details)
  {
    CLog::Log(LOGERROR, "IMDB: Invalid xml file");
    return false;
  }

  CStdString strTemp;
  GetString(details, "title", movieDetails.m_strTitle);
  if (GetString(details, "rating", strTemp)) movieDetails.m_fRating = (float)atof(strTemp);
  if (GetString(details, "year", strTemp)) movieDetails.m_iYear = atoi(strTemp);
  if (GetString(details, "top250", strTemp)) movieDetails.m_iTop250 = atoi(strTemp);

  GetString(details, "votes", movieDetails.m_strVotes);
  GetString(details, "cast", movieDetails.m_strCast);
  GetString(details, "outline", movieDetails.m_strPlotOutline);
  GetString(details, "runtime", movieDetails.m_strRuntime);
  GetString(details, "thumb", movieDetails.m_strPictureURL);
  GetString(details, "tagline", movieDetails.m_strTagLine);
  GetString(details, "genre", movieDetails.m_strGenre);
  GetString(details, "credits", movieDetails.m_strWritingCredits);
  GetString(details, "director", movieDetails.m_strDirector);
  GetString(details, "plot", movieDetails.m_strPlot);

  return true;
}

bool CIMDB::LoadDetails(const CStdString& strIMDB, CIMDBMovie &movieDetails)
{
  CStdString strXMLFile;
  strXMLFile.Format("%s\\imdb\\%s.xml", g_stSettings.m_szAlbumDirectory, strIMDB.c_str());
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  movieDetails.m_strIMDBNumber = strIMDB;
  if (doc.LoadFile(strXMLFile) && ParseDetails(doc, movieDetails))
  { // excellent!
    return true;
  }
  TiXmlBase::SetCondenseWhiteSpace(true);
  // oh no - let's try and redownload them.
  CGUIDialogProgress *pDlgProgress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDlgProgress)
    return false;
  pDlgProgress->SetHeading(198);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, movieDetails.m_strTitle);
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
  pDlgProgress->Progress();
  CIMDBUrl url;
  url.m_strTitle = movieDetails.m_strTitle;
  url.m_strURL.Format("http://%s/title/%s", g_stSettings.m_szIMDBurl, strIMDB.c_str());
  bool ret = GetDetails(url, movieDetails, pDlgProgress);
  pDlgProgress->Close();
  return ret;
}

bool CIMDB::Download(const CStdString &strURL, const CStdString &strFileName)
{
  CStdString strHTML;
  if (!m_http.Download(strURL, strFileName))
  {
    CLog::Log(LOGERROR, "failed to download %s -> %s", strURL.c_str(), strFileName.c_str());
    return false;
  }

  return true;
}

void CIMDBMovie::Reset()
{
  m_strDirector = "";
  m_strWritingCredits = "";
  m_strGenre = "";
  m_strTagLine = "";
  m_strPlotOutline = "";
  m_strPlot = "";
  m_strPictureURL = "";
  m_strTitle = "";
  m_strVotes = "";
  m_strCast = "";
  m_strSearchString = "";
  m_strFile = "";
  m_strPath = "";
  m_strDVDLabel = "";
  m_strIMDBNumber = "";
  m_iTop250 = 0;
  m_iYear = 0;
  m_fRating = 0.0f;
  m_bWatched = false;
}

void CIMDBMovie::Save(const CStdString &strFileName)
{}

bool CIMDBMovie::Load(const CStdString& strFileName)
{
  return true;
}


void CIMDB::RemoveAllAfter(char* szMovie, const char* szSearch)
{
  char* pPtr = strstr(szMovie, szSearch);
  if (pPtr) *pPtr = 0;
}

void CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL)
{
  // char szURL[1024];
  char szMovie[1024];
  CIMDBUrl url;
  bool bSkip = false;
  int ipos = 0;

  // skip extension
  int imax = 0;
  for (int i = 0; i < strMovie.size();i++)
  {
    if (strMovie[i] == '.') imax = i;
    if (i + 2 < strMovie.size())
    {
      // skip -CDx. also
      if (strMovie[i] == '-')
      {
        if (strMovie[i + 1] == 'C' || strMovie[i + 1] == 'c')
        {
          if (strMovie[i + 2] == 'D' || strMovie[i + 2] == 'd')
          {
            imax = i;
            break;
          }
        }
      }
    }
  }
  if (!imax) imax = strMovie.size();
  for (int i = 0; i < imax;i++)
  {
    // Removing arbitrary numbers like this destroys lookup of movies
    // such as "1942"
    // TODO: Redo this routine entirely.
/*    for (int c = 0;isdigit(strMovie[i + c]);c++)
    {
      if (c == 3)
      {
        i += 4;
        break;
      }
    }*/
    char kar = strMovie[i];
    if (kar == '.') kar = ' ';
    if (kar == 32) kar = '+';
    if (kar == '[' || kar == '(' ) bSkip = true;   //skip everthing between () and []
    else if (kar == ']' || kar == ')' ) bSkip = false;
    else if (!bSkip)
    {
      if (ipos > 0)
      {
        if (!isalnum(kar))
        {
          if (szMovie[ipos - 1] != '+')
            kar = '+';
          else
            kar = '.';
        }
      }
      if (isalnum(kar) || kar == ' ' || kar == '+')
      {
        szMovie[ipos] = kar;
        szMovie[ipos + 1] = 0;
        ipos++;
      }
    }
  }

  CStdString strTmp = szMovie;
  strTmp = strTmp.ToLower();
  strTmp = strTmp.Trim();
  strcpy(szMovie, strTmp.c_str());

  RemoveAllAfter(szMovie, " divx ");
  RemoveAllAfter(szMovie, " xvid ");
  RemoveAllAfter(szMovie, " dvd ");
  RemoveAllAfter(szMovie, " svcd ");
  RemoveAllAfter(szMovie, " ac3 ");
  RemoveAllAfter(szMovie, " ogg ");
  RemoveAllAfter(szMovie, " ogm ");
  RemoveAllAfter(szMovie, " internal ");
  RemoveAllAfter(szMovie, " fragment ");
  RemoveAllAfter(szMovie, " dvdrip ");
  RemoveAllAfter(szMovie, " proper ");
  RemoveAllAfter(szMovie, " limited ");
  RemoveAllAfter(szMovie, " rerip ");

  RemoveAllAfter(szMovie, "+divx+");
  RemoveAllAfter(szMovie, "+xvid+");
  RemoveAllAfter(szMovie, "+dvd+");
  RemoveAllAfter(szMovie, "+svcd+");
  RemoveAllAfter(szMovie, "+ac3+");
  RemoveAllAfter(szMovie, "+ogg+");
  RemoveAllAfter(szMovie, "+ogm+");
  RemoveAllAfter(szMovie, "+internal+");
  RemoveAllAfter(szMovie, "+fragment+");
  RemoveAllAfter(szMovie, "+dvdrip+");
  RemoveAllAfter(szMovie, "+proper+");
  RemoveAllAfter(szMovie, "+limited+");
  RemoveAllAfter(szMovie, "+rerip+");

  // sprintf(szURL,"http://us.imdb.com/Tsearch?title=%s", szMovie);
  // strURL = szURL;
  strURL.Format("http://%s/Tsearch?title=%s", g_stSettings.m_szIMDBurl, szMovie);
}

// threaded functions
void CIMDB::Process()
{
  m_found = false;
  if (m_state == FIND_MOVIE)
  {
    if (!FindMovie(m_strMovie, m_movieList))
      CLog::Log(LOGERROR, "IMDb::Error looking up movie %s", m_strMovie.c_str());
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "IMDb::Error getting movie details from %s", m_url.m_strURL.c_str());
  }
  m_found = true;
}

bool CIMDB::FindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movieList, CGUIDialogProgress *pProgress /* = NULL */)
{
  m_strMovie = strMovie;
  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
    }
    // transfer to our movielist
    movieList.clear();
    for (unsigned int i=0; i < m_movieList.size(); i++)
      movieList.push_back(m_movieList[i]);
    m_movieList.clear();
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalFindMovie(strMovie, movieList);
}

bool CIMDB::GetDetails(const CIMDBUrl &url, CIMDBMovie &movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  m_url = url;
  m_movieDetails = movieDetails;
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
    }
    movieDetails = m_movieDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalGetDetails(url, movieDetails);
}

void CIMDB::CloseThread()
{
  m_http.Cancel();
  StopThread();
  m_state = DO_NOTHING;
  m_found = false;
}
