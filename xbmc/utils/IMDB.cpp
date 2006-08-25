// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "IMDB.h"
#include "../util.h"
#include "HTMLUtil.h"
#include "../FileSystem/FileCurl.h"
#include "XMLUtils.h"
#include "RegExp.h"

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

	CStdString strURL, strHTML, strYear;
	GetURL(strMovie, strURL, strYear);

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

			char cFoundYear = 0; //0 no year available 1: not yet found 2: found
			if (!strYear.Equals("")) cFoundYear=1; // Year exists in title

			if ((cFoundYear>0) && (!url.m_strTitle.substr(max(url.m_strTitle.length()-5,0),4).compare(strYear)))
			{
				if (cFoundYear==1) // not previously found
				{
					movielist.clear(); // First time right year found, remove previous entries
					cFoundYear=2; // year found
				}
				movielist.push_back(url);
			}
			else if (cFoundYear<2) // Only add when not yet filtering on year
      movielist.push_back(url);
    }
  }
  return true;
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
  movieDetails.m_strMPAARating = strLocNotAvail;
  movieDetails.m_iYear = 0;
  movieDetails.m_fRating = 0.0;
  movieDetails.m_strVotes = strLocNotAvail;
  movieDetails.m_strCast = strLocNotAvail;
  movieDetails.m_iTop250 = 0;

  if (!m_http.Get(strURL, strHTML) || strHTML.size() == 0)
    return false;

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

  // grab our plot summary as well (if it's available)
  // http://www.imdb.com/Title/ttIMDBNUMBER/plotsummary
  CStdString strPlotURL = "http://www.imdb.com/title/" + movieDetails.m_strIMDBNumber + "/plotsummary";
  m_http.Get(strPlotURL, strPlotHTML);

  // now grab our details using the dll
  CStdString strXML;
  char *szXML = strXML.GetBuffer(50000);
  if (!m_dll.IMDbGetDetails(szXML, strHTML.c_str(), strPlotHTML.c_str()))
  {
    strXML.ReleaseBuffer();
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }
  strXML.ReleaseBuffer();

  // abit uggly, but should work. would have been better if parset
  // set the charset of the xml, and we made use of that
  g_charsetConverter.stringCharsetToUtf8(strXML);

  // save the xml file for later reading...
  CFile file;
  CStdString strXMLFile;
  strXMLFile.Format("%s\\%s.xml", g_settings.GetIMDbFolder().c_str(), movieDetails.m_strIMDBNumber.c_str());
  if (file.OpenForWrite(strXMLFile))
  {
    file.Write(strXML.c_str(), strXML.size());
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
  movieDetails.m_strMPAARating = strLocNotAvail;
  movieDetails.m_iTop250 = 0;

  TiXmlNode *details = doc.FirstChild( "details" );

  if (!details)
  {
    CLog::Log(LOGERROR, "IMDB: Invalid xml file");
    return false;
  }

  CStdString strTemp;
  XMLUtils::GetString(details, "title", movieDetails.m_strTitle);
  if (XMLUtils::GetString(details, "rating", strTemp)) movieDetails.m_fRating = (float)atof(strTemp);
  if (XMLUtils::GetString(details, "year", strTemp)) movieDetails.m_iYear = atoi(strTemp);
  if (XMLUtils::GetString(details, "top250", strTemp)) movieDetails.m_iTop250 = atoi(strTemp);

  XMLUtils::GetString(details, "votes", movieDetails.m_strVotes);
  XMLUtils::GetString(details, "cast", movieDetails.m_strCast);
  XMLUtils::GetString(details, "outline", movieDetails.m_strPlotOutline);
  XMLUtils::GetString(details, "runtime", movieDetails.m_strRuntime);
  XMLUtils::GetString(details, "thumb", movieDetails.m_strPictureURL);
  XMLUtils::GetString(details, "tagline", movieDetails.m_strTagLine);
  XMLUtils::GetString(details, "genre", movieDetails.m_strGenre);
  XMLUtils::GetString(details, "credits", movieDetails.m_strWritingCredits);
  XMLUtils::GetString(details, "director", movieDetails.m_strDirector);
  XMLUtils::GetString(details, "plot", movieDetails.m_strPlot);
  XMLUtils::GetString(details, "mpaa", movieDetails.m_strMPAARating);

  return true;
}

bool CIMDB::LoadDetails(const CStdString& strIMDB, CIMDBMovie &movieDetails)
{
  CStdString strXMLFile;
  strXMLFile.Format("%s\\%s.xml", g_settings.GetIMDbFolder().c_str(), strIMDB.c_str());
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
  pDlgProgress->StartModal();
  pDlgProgress->Progress();
  CIMDBUrl url;
  url.m_strTitle = movieDetails.m_strTitle;
  url.m_strURL.Format("http://%s/title/%s", g_advancedSettings.m_imdbAddress.c_str(), strIMDB.c_str());
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
  m_strMPAARating = "";
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

// TODO: Make this user-configurable?
void CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL, CStdString& strYear)
{
  char szMovie[1024];
  char szYear[5];

  CStdString strMovieNoExtension = strMovie;
  CUtil::RemoveExtension(strMovieNoExtension);

  // replace whitespace with +
  strMovieNoExtension.Replace(".","+");
  strMovieNoExtension.Replace("-","+");
  strMovieNoExtension.Replace(" ","+");

  // lowercase
  strMovieNoExtension = strMovieNoExtension.ToLower();

  // default to movie name begin complete filename, no year
  strcpy(szMovie, strMovieNoExtension.c_str());
  strcpy(szYear,"");

  CRegExp reYear;
  reYear.RegComp("(.+)\\+\\(?(19[0-9][0-9]|200[0-9])\\)?(\\+.*)?");
  if (reYear.RegFind(szMovie) >= 0)
  {
    char *pMovie = reYear.GetReplaceString("\\1");
    char *pYear = reYear.GetReplaceString("\\2");
    strcpy(szMovie,pMovie);
    strcpy(szYear,pYear);

    if (pMovie) free(pMovie);
    if (pYear) free(pYear);
          }

  CRegExp reTags;
  reTags.RegComp("(.*)\\+(ac3|custom|dc|divx|dsr|dsrip|dutch|dvd|dvdrip|dvdscr|fragment|fs|hdtv|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|se|svcd|swedish|unrated|ws|xvid|cd[1-9]|\\[.*\\])(\\+.*)?");

  CStdString strTemp;
  while (reTags.RegFind(szMovie) >= 0)
    {
    char *pFN = reTags.GetReplaceString("\\1");
    strcpy(szMovie,pFN);
    if (pFN) free(pFN);
    }

  strURL.Format("http://%s/find?s=tt;q=%s", g_advancedSettings.m_imdbAddress.c_str(), szMovie);

  strYear = szYear;
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
  //CLog::Log(LOGDEBUG,"CIMDB::FindMovie(%s)", strMovie.c_str());
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
      Sleep(1);
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
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
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
      Sleep(1);
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
