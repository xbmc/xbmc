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
      if (id && id->FirstChild())
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
  movieDetails.m_cast.clear();
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
  movieDetails.m_cast.clear();
  movieDetails.m_strMPAARating = strLocNotAvail;
  movieDetails.m_iTop250 = 0;

  TiXmlNode *details = doc.FirstChild( "details" );

  if (!details)
  {
    CLog::Log(LOGERROR, "IMDB: Invalid xml file");
    return false;
  }

  movieDetails.Load(details);

  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strTitle);
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strPlotOutline);
  g_charsetConverter.stringCharsetToUtf8(movieDetails.m_strPlot);
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
  m_cast.clear();
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

bool CIMDBMovie::Save(TiXmlNode *node)
{
  if (!node) return false;

  // we start with a <movie> tag
  TiXmlElement movieElement("movie");
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetInt(movie, "year", m_iYear);
  XMLUtils::SetInt(movie, "top250", m_iTop250);
  XMLUtils::SetString(movie, "votes", m_strVotes);
  XMLUtils::SetString(movie, "outline", m_strPlotOutline);
  XMLUtils::SetString(movie, "plot", m_strPlot);
  XMLUtils::SetString(movie, "tagline", m_strTagLine);
  XMLUtils::SetString(movie, "runtime", m_strRuntime);
  XMLUtils::SetString(movie, "thumb", m_strPictureURL);
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetBoolean(movie, "watched", m_bWatched);
  XMLUtils::SetString(movie, "searchstring", m_strSearchString);
  XMLUtils::SetString(movie, "file", m_strFile);
  XMLUtils::SetString(movie, "path", m_strPath);
  XMLUtils::SetString(movie, "imdbnumber", m_strIMDBNumber);
  XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::SetString(movie, "genre", m_strGenre);
  XMLUtils::SetString(movie, "credits", m_strWritingCredits);
  XMLUtils::SetString(movie, "director", m_strDirector);

  // cast
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("actor");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(it->first);
    actorNode->InsertEndChild(name);
    TiXmlElement role("role");
    TiXmlNode *roleNode = node->InsertEndChild(role);
    TiXmlText character(it->second);
    roleNode->InsertEndChild(character);
  }
  return true;
}

bool CIMDBMovie::Load(const TiXmlNode *movie)
{
  if (!movie) return false;
  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetInt(movie, "year", m_iYear);
  XMLUtils::GetInt(movie, "top250", m_iTop250);
  XMLUtils::GetString(movie, "votes", m_strVotes);
  XMLUtils::GetString(movie, "outline", m_strPlotOutline);
  XMLUtils::GetString(movie, "plot", m_strPlot);
  XMLUtils::GetString(movie, "tagline", m_strTagLine);
  XMLUtils::GetString(movie, "runtime", m_strRuntime);
  XMLUtils::GetString(movie, "thumb", m_strPictureURL);
  XMLUtils::GetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::GetBoolean(movie, "watched", m_bWatched);
  XMLUtils::GetString(movie, "searchstring", m_strSearchString);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "imdbnumber", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);

  XMLUtils::GetString(movie, "genre", m_strGenre);
  XMLUtils::GetString(movie, "credits", m_strWritingCredits);
  XMLUtils::GetString(movie, "director", m_strDirector);

  // cast
  const TiXmlNode *node = movie->FirstChild("actor");
  while (node)
  {
    const TiXmlNode *actor = node->FirstChild("name");
    if (actor && actor->FirstChild())
    {
      CStdString name = actor->FirstChild()->Value();
      CStdString role;
      const TiXmlNode *roleNode = node->FirstChild("role");
      if (roleNode && roleNode->FirstChild())
        role = roleNode->FirstChild()->Value();
      m_cast.push_back(make_pair(name, role));
    }
    node = node->NextSibling("actor");
  }
  if (m_cast.empty())
  { // old method for back-compatibility
    CStdString cast;
    XMLUtils::GetString(movie, "cast", cast);
    vector<CStdString> vecCast;
    int iNumItems = StringUtils::SplitString(cast, "\n", vecCast);
    for (unsigned int i = 0; i < vecCast.size(); i++)
    {
      int iPos = vecCast[i].Find(" as ");
      if (iPos > 0)
        m_cast.push_back(make_pair(vecCast[i].Left(iPos), vecCast[i].Mid(iPos + 4)));
    }
  }
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
void CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL, CStdString& strYear)
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
  reTags.RegComp("\\+(ac3|custom|dc|divx|dsr|dsrip|dutch|dvd|dvdrip|dvdscr|fragment|fs|hdtv|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|se|svcd|swedish|unrated|ws|xvid|cd[1-9]|\\[.*\\])(\\+|$)");

  CStdString strTemp;
  int i=0;
  CStdString strSearch = szMovie;
  if ((i=reTags.RegFind(strSearch.c_str())) >= 0) // new logic - select the crap then drop anything to the right of it
  {
    m_parser.m_param[0] = strSearch.Mid(0,i);
 }
  else
    m_parser.m_param[0] = szMovie;

  strURL = m_parser.Parse("CreateSearchUrl");

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
