// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "IMDB.h"
#include "../util.h"
#include "HTMLUtil.h"
#include "XMLUtils.h"
#include "RegExp.h"
#include "ScraperParser.h"

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
  // load our scraper xml
  if (!m_parser.Load("Q:\\system\\scrapers\\video\\"+m_info.strPath))
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
  
  m_parser.m_param[0] = strHTML;
  m_parser.m_param[1] = strURL;
  CStdString strXML = m_parser.Parse("GetSearchResults");
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str());
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
    return false;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement *movie = docHandle.FirstChild( "results" ).FirstChild( "movie" ).Element();

  int iYear = atoi(strYear);

  for ( movie; movie; movie = movie->NextSiblingElement() )
  {
    url.m_strURL.clear();
    TiXmlNode *title = movie->FirstChild("title");
    TiXmlNode *link = movie->FirstChild("url");
    TiXmlNode *year = movie->FirstChild("year");
    TiXmlNode* id = movie->FirstChild("id");
    if (title && title->FirstChild() && link && link->FirstChild())
    {
      g_charsetConverter.stringCharsetToUtf8(title->FirstChild()->Value(),url.m_strTitle);
      url.m_strURL.push_back(link->FirstChild()->Value());
      while ((link = link->NextSibling("url")))
      {
        url.m_strURL.push_back(link->FirstChild()->Value());
      }
      if (id)
        url.m_strID = id->FirstChild()->Value();
      // if source contained a distinct year, only allow those
      if(iYear != 0)
      {
        if(year && year->FirstChild())
        { // sweet scraper provided a year
          if(iYear != atoi(year->FirstChild()->Value()))
            continue;
        }
        else if(url.m_strTitle.length() >= 6)
        { // imdb normally puts year at end of title within ()
          if(url.m_strTitle.at(url.m_strTitle.length()-1) == ')'
          && url.m_strTitle.at(url.m_strTitle.length()-6) == '(')
          {
            int iYear2 = atoi(url.m_strTitle.Right(5).Left(4).c_str());
            if( iYear2 != 0 && iYear != iYear2)
              continue;
          }
        }
      }

      movielist.push_back(url);
    }
  }
  return true;
}

bool CIMDB::InternalGetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails)
{
  // load our scraper xml
  if (!m_parser.Load("q:\\system\\scrapers\\video\\"+m_info.strPath))
    return false;

  std::vector<CStdString> strHTML;

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
  movieDetails.m_strIMDBNumber = url.m_strID;
  movieDetails.m_bWatched = false;

  for (unsigned int i=0;i<url.m_strURL.size();++i)
  {
    CStdString strCurrHTML;
    CStdString strU = url.m_strURL[i];
    if (!m_http.Get(strU,strCurrHTML) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (int i=0;i<strHTML.size();++i)
    m_parser.m_param[i] = strHTML[i];

  m_parser.m_param[strHTML.size()] = url.m_strID;

  CStdString strXML = m_parser.Parse("GetDetails");
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  // abit uggly, but should work. would have been better if parset
  // set the charset of the xml, and we made use of that
  g_charsetConverter.stringCharsetToUtf8(strXML);

    // ok, now parse the xml file
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  doc.Parse(strXML.c_str());
  if (!doc.RootElement())
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
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strTitle);
  if (XMLUtils::GetString(details, "rating", strTemp)) movieDetails.m_fRating = (float)atof(strTemp);
  if (XMLUtils::GetString(details, "year", strTemp)) movieDetails.m_iYear = atoi(strTemp);
  if (XMLUtils::GetString(details, "top250", strTemp)) movieDetails.m_iTop250 = atoi(strTemp);

  XMLUtils::GetString(details, "votes", movieDetails.m_strVotes);
  XMLUtils::GetString(details, "cast", movieDetails.m_strCast);
  XMLUtils::GetString(details, "outline", movieDetails.m_strPlotOutline);
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strPlotOutline);
  
  XMLUtils::GetString(details, "runtime", movieDetails.m_strRuntime);
  XMLUtils::GetString(details, "thumb", movieDetails.m_strPictureURL);
  XMLUtils::GetString(details, "tagline", movieDetails.m_strTagLine);
  XMLUtils::GetString(details, "genre", movieDetails.m_strGenre);
  XMLUtils::GetString(details, "credits", movieDetails.m_strWritingCredits);
  XMLUtils::GetString(details, "director", movieDetails.m_strDirector);
  XMLUtils::GetString(details, "plot", movieDetails.m_strPlot);
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strPlot);
  
  XMLUtils::GetString(details, "mpaa", movieDetails.m_strMPAARating);
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strMPAARating);
  
  CHTMLUtil::RemoveTags(movieDetails.m_strPlot);

  return true;
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

bool CIMDB::LoadXML(const CStdString& strXMLFile, CIMDBMovie &movieDetails, bool bDownload /* = true */)
{
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  if (doc.LoadFile(strXMLFile) && ParseDetails(doc, movieDetails))
  { // excellent!
    return true;
  }
  if (!bDownload)
    return true;

  return false;
}

void CIMDB::RemoveAllAfter(char* szMovie, const char* szSearch)
{
  char* pPtr = strstr(szMovie, szSearch);
  if (pPtr) *pPtr = 0;
}

// TODO: Make this user-configurable?
const CStdString CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL, CStdString& strYear)
{
  char szMovie[1024];
  char szYear[5];

  CStdString strMovieNoExtension = strMovie;
  //don't assume movie name is a file with an extension
  //CUtil::RemoveExtension(strMovieNoExtension);

  // replace whitespace with +
  strMovieNoExtension.Replace(".","+");
  strMovieNoExtension.Replace("-","+");
  strMovieNoExtension.Replace(" ","+");

  // lowercase
  strMovieNoExtension = strMovieNoExtension.ToLower();

  // strip off 'the' from start of title - confuses the searches
  if (strMovieNoExtension.Mid(0,4).Equals("the+"))
    strMovieNoExtension = strMovieNoExtension.Mid(4);

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

    m_parser.m_param[0] = szMovie;
    strURL = m_parser.Parse("CreateSearchUrl");

  strYear = szYear;
  return szMovie;
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
      CLog::Log(LOGERROR, "IMDb::Error getting movie details from %s", m_url.m_strURL[0].c_str());
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
