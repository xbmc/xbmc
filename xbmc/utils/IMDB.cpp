/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


// IMDB.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IMDB.h"
#include "Util.h"
#include "HTMLUtil.h"
#include "XMLUtils.h"
#include "RegExp.h"
#include "ScraperParser.h"
#include "NfoFile.h"
#include "GUIDialogProgress.h"
#include "Settings.h"
#include "fstrcmp.h"

using namespace HTML;
using namespace std;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CIMDB::CIMDB()
{
  m_retry = false;
}

CIMDB::~CIMDB()
{
}

bool CIMDB::InternalFindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movielist, const CStdString& strFunction, CScraperUrl* pUrl)
{
  movielist.clear();

  CScraperUrl scrURL;

  CStdString movieTitle = strMovie;
  CStdString movieYear;
  GetCleanNameAndYear(movieTitle, movieYear);

  if (!pUrl)
  {
    if (m_parser.HasFunction("CreateSearchUrl"))
    {
      GetURL(strMovie, movieTitle, movieYear, scrURL);
    }
    else if (m_info.strContent.Equals("musicvideos"))
    {
    if (!m_parser.HasFunction("FileNameScrape"))
       return false;

      CScraperUrl scrURL("filenamescrape");
      scrURL.strTitle = strMovie;
      movielist.push_back(scrURL);
      return true;
    }
    if (scrURL.m_xml.IsEmpty())
      return false;
  }
  else
    scrURL = *pUrl;  
  
  vector<CStdString> strHTML;
  for (unsigned int i=0;i<scrURL.m_url.size();++i)
  {
    CStdString strCurrHTML;
    if (!CScraperUrl::Get(scrURL.m_url[i],strCurrHTML,m_http) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
    m_parser.m_param[i] = strHTML[i];
  m_parser.m_param[strHTML.size()] = scrURL.m_url[0].m_url;
  CStdString strXML = m_parser.Parse(strFunction,&m_info.settings);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return false;
  }

  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return false;
  }
 
  TiXmlHandle docHandle( &doc );

  TiXmlElement* xurl = doc.RootElement()->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {
      CScraperUrl scrURL(xurl);
      InternalFindMovie(strMovie,movielist,szFunction,&scrURL);
    }
    xurl = xurl->NextSiblingElement("url");
  }

  TiXmlElement *movie = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!movie)
    return false;

  while (movie)
  {
    CScraperUrl url;
    TiXmlNode *title = movie->FirstChild("title");
    TiXmlElement *link = movie->FirstChildElement("url");
    TiXmlNode *year = movie->FirstChild("year");
    TiXmlNode* id = movie->FirstChild("id");
    TiXmlNode* language = movie->FirstChild("language");
    if (title && title->FirstChild() && link && link->FirstChild())
    {
      url.strTitle = title->FirstChild()->Value();
      while (link && link->FirstChild())
      {
        url.ParseElement(link);
        link = link->NextSiblingElement("url");
      }
      if (id && id->FirstChild())
        url.strId = id->FirstChild()->Value();

      // calculate the relavance of this hit
      CStdString compareTitle = url.strTitle;
      compareTitle.ToLower();
      CStdString matchTitle = movieTitle;
      matchTitle.ToLower();
      // see if we need to add year information
      CStdString compareYear;
      if (year && year->FirstChild())
        compareYear = year->FirstChild()->Value();
      if (!movieYear.IsEmpty() && !compareYear.IsEmpty())
      {
        matchTitle.AppendFormat(" (%s)", movieYear.c_str());
        compareTitle.AppendFormat(" (%s)", compareYear.c_str());
      }
      url.relevance = fstrcmp(matchTitle.c_str(), compareTitle.c_str(), 0);
      // reconstruct a title for the user
      CStdString title = url.strTitle;
      if (!compareYear.IsEmpty())
        title.AppendFormat(" (%s)", compareYear.c_str());
      if (language && language->FirstChild())
        title.AppendFormat(" (%s)", language->FirstChild()->Value());
      url.strTitle = title;
      movielist.push_back(url);
    }
    movie = movie->NextSiblingElement();
  }

  return true;
}

bool CIMDB::RelevanceSortFunction(const CScraperUrl &left, const CScraperUrl &right)
{
  return left.relevance > right.relevance;
}

bool CIMDB::InternalGetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& details)
{
  IMDB_EPISODELIST temp;
  for(unsigned int i=0; i < url.m_url.size(); i++)
  {
    CStdString strHTML;
    if (!CScraperUrl::Get(url.m_url[i],strHTML,m_http) || strHTML.size() == 0)
    {
      CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
      if (temp.size() > 0 || (i == 0 && url.m_url.size() > 1)) // use what was fetched
        continue;

      return false;
    }
    m_parser.m_param[0] = strHTML;
    m_parser.m_param[1] = url.m_url[i].m_url;

    CStdString strXML = m_parser.Parse("GetEpisodeList",&m_info.settings);
    if (strXML.IsEmpty())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
      if (temp.size() > 0 || (i == 0 && url.m_url.size() > 1)) // use what was fetched
        continue;

      return false;
    }
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(strXML.c_str());
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      return false;
    }

    if (!XMLUtils::HasUTF8Declaration(strXML))
      g_charsetConverter.unknownToUTF8(strXML);

    TiXmlHandle docHandle( &doc );
    TiXmlElement *movie = docHandle.FirstChild( "episodeguide" ).FirstChild( "episode" ).Element();

    while (movie)
    {
      TiXmlNode *title = movie->FirstChild("title");
      TiXmlElement *link = movie->FirstChildElement("url");
      TiXmlNode *epnum = movie->FirstChild("epnum");
      TiXmlNode *season = movie->FirstChild("season");
      TiXmlNode* id = movie->FirstChild("id");
      if (link && link->FirstChild() && epnum && epnum->FirstChild() && season && season->FirstChild())
      {
        CScraperUrl url2;
        if (title && title->FirstChild())
          url2.strTitle = title->FirstChild()->Value();
        else
          url2.strTitle = g_localizeStrings.Get(416);

        while (link && link->FirstChild())
        {
          url2.ParseElement(link);
          link = link->NextSiblingElement("url");
        }

        if (id && id->FirstChild())
          url2.strId = id->FirstChild()->Value();
        pair<int,int> key(atoi(season->FirstChild()->Value()),atoi(epnum->FirstChild()->Value()));
        temp.insert(make_pair(key,url2));
      }
      movie = movie->NextSiblingElement();
    }
  }

  // find minimum in each season
  map<int,int> min;
  for (IMDB_EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter )
  {
    if ((signed int) min.size() == (iter->first.first -1))
      min.insert(iter->first);
    else if (iter->first.second < min[iter->first.first])
      min[iter->first.first] = iter->first.second;
  }
  // correct episode numbers
  for (IMDB_EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter )
  {
    int episode=iter->first.second - min[iter->first.first];
    if (min[iter->first.first] > 0)
      episode++;
    pair<int,int> key(iter->first.first,episode);
    details.insert(make_pair(key,iter->second));
  }

  return true;
}

bool CIMDB::InternalGetDetails(const CScraperUrl& url, CVideoInfoTag& movieDetails, const CStdString& strFunction)
{
  vector<CStdString> strHTML;

  for (unsigned int i=0;i<url.m_url.size();++i)
  {
    CStdString strCurrHTML;
    if (url.m_xml.Equals("filenamescrape"))
      return ScrapeFilename(movieDetails.m_strFileNameAndPath,movieDetails);
    if (!CScraperUrl::Get(url.m_url[i],strCurrHTML,m_http) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
    m_parser.m_param[i] = strHTML[i];

  m_parser.m_param[strHTML.size()] = url.strId;
  m_parser.m_param[strHTML.size()+1] = url.m_url[0].m_url;

  CStdString strXML = m_parser.Parse(strFunction,&m_info.settings);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site [%s]",__FUNCTION__,url.m_url[0].m_url.c_str());
    return false;
  }

  // abit ugly, but should work. would have been better if parser
  // set the charset of the xml, and we made use of that
  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

    // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return false;
  }

  bool ret = ParseDetails(doc, movieDetails);
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* xurl = pRoot->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {
      CScraperUrl scrURL(xurl);
      InternalGetDetails(scrURL,movieDetails,szFunction);
    }
    xurl = xurl->NextSiblingElement("url");
  }

  return ret;
}

bool CIMDB::ParseDetails(TiXmlDocument &doc, CVideoInfoTag &movieDetails)
{
  TiXmlHandle docHandle( &doc );
  TiXmlElement *details = docHandle.FirstChild( "details" ).Element();

  if (!details)
  {
    CLog::Log(LOGERROR, "%s: Invalid xml file",__FUNCTION__);
    return false;
  }

  // set chaining to true here as this is called by our scrapers
  movieDetails.Load(details, true);

  CHTMLUtil::RemoveTags(movieDetails.m_strPlot);

  return true;
}

bool CIMDB::LoadXML(const CStdString& strXMLFile, CVideoInfoTag &movieDetails, bool bDownload /* = true */)
{
  TiXmlDocument doc;

  movieDetails.Reset();
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

void CIMDB::GetURL(const CStdString &movieFile, const CStdString &movieName, const CStdString &movieYear, CScraperUrl& scrURL)
{
  bool bOkay = false;
  if (m_info.strContent.Equals("musicvideos"))
  {
    CVideoInfoTag tag;
    if (ScrapeFilename(movieFile,tag))
    {
      m_parser.m_param[0] = tag.m_strArtist;
      m_parser.m_param[1] = tag.m_strTitle;
      CUtil::URLEncode(m_parser.m_param[0]);
      CUtil::URLEncode(m_parser.m_param[1]);
      bOkay = true;
    }
  }
  if (!bOkay)
  {
    if (!movieYear.IsEmpty())
      m_parser.m_param[1] = movieYear;

    // convert to the encoding requested by the parser
    g_charsetConverter.utf8To(m_parser.GetSearchStringEncoding(), movieName, m_parser.m_param[0]);
    CUtil::URLEncode(m_parser.m_param[0]);
  }
  scrURL.ParseString(m_parser.Parse("CreateSearchUrl",&m_info.settings));
}

// TODO: Make this user-configurable?
void CIMDB::GetCleanNameAndYear(CStdString &strMovieName, CStdString &strYear)
{
#define SEP " _\\.\\(\\)\\[\\]\\-"
  CRegExp reYear;
  reYear.RegComp("(.+[^"SEP"])["SEP"]+(19[0-9][0-9]|20[0-1][0-9])(["SEP"]|$)");

  strMovieName.ToLower();

  if (reYear.RegFind(strMovieName.c_str()) >= 0)
  {
    char *pMovie = reYear.GetReplaceString("\\1");
    char *pYear = reYear.GetReplaceString("\\2");

    if(pMovie)
    {
      strMovieName = pMovie;
      free(pMovie);
    }
    if(pYear)
    {
      strYear = pYear;
      free(pYear);
    }
  }
  // get clean string
  CUtil::CleanString(strMovieName,true);
}

// threaded functions
void CIMDB::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = false;
  m_retry = false;
  if (m_state == FIND_MOVIE)
  {
    if (!FindMovie(m_strMovie, m_movieList))
    {
      // retry without replacing '.' and '-' if searching for a tvshow
      if (m_info.strContent.Equals("tvshows"))
      {
        m_retry = true;
        if (!FindMovie(m_strMovie, m_movieList))
          CLog::Log(LOGERROR, "%s: Error looking up tvshow %s", __FUNCTION__, m_strMovie.c_str());
      }
      else
        CLog::Log(LOGERROR, "%s: Error looking up movie %s", __FUNCTION__, m_strMovie.c_str());
    }
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "%s: Error getting movie details from %s", __FUNCTION__,m_url.m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_DETAILS)
  {
    if (!GetEpisodeDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "%s: Error getting movie details from %s", __FUNCTION__, m_url.m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_LIST)
  {
    if (!GetEpisodeList(m_url, m_episode))
      CLog::Log(LOGERROR, "%s: Error getting episode details from %s", __FUNCTION__, m_url.m_url[0].m_url.c_str());
  }
  m_found = true;
}

bool CIMDB::FindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movieList, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::FindMovie(%s)", strMovie.c_str());

  // load our scraper xml
  if (!m_parser.Load(CUtil::AddFileToFolder("special://xbmc/system/scrapers/video/", m_info.strPath)))
    return false;
  CScraperParser::ClearCache();

  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_strMovie = strMovie;
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
  
  // unthreaded
  bool success = InternalFindMovie(strMovie, movieList);
  // sort our movie list by fuzzy match
  std::sort(movieList.begin(), movieList.end(), RelevanceSortFunction);
  return success;
}

bool CIMDB::GetDetails(const CScraperUrl &url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;
  // load our scraper xml
  if (!m_parser.Load("special://xbmc/system/scrapers/video/"+m_info.strPath))
    return false;

  // fill in the defaults
  movieDetails.Reset();
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

bool CIMDB::GetEpisodeDetails(const CScraperUrl &url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_DETAILS;
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
    return InternalGetDetails(url, movieDetails, "GetEpisodeDetails");
}

bool CIMDB::GetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_episode = movieDetails;

  // load our scraper xml
  if (!m_parser.Load(CUtil::AddFileToFolder("special://xbmc/system/scrapers/video/", m_info.strPath)))
    return false;

  // fill in the defaults
  movieDetails.clear();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_LIST;
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
    movieDetails = m_episode;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalGetEpisodeList(url, movieDetails);
}

void CIMDB::CloseThread()
{
  m_http.Cancel();
  StopThread();
  m_http.Reset();
  m_state = DO_NOTHING;
  m_found = false;
}

bool CIMDB::ScrapeFilename(const CStdString& strFileName, CVideoInfoTag& details)
{
  m_parser.m_param[0] = strFileName;

  CUtil::RemoveExtension(m_parser.m_param[0]);
  m_parser.m_param[0].Replace("_"," ");
  CStdString strResult = m_parser.Parse("FileNameScrape",&m_info.settings);
  TiXmlDocument doc;
  doc.Parse(strResult.c_str());
  if (doc.RootElement())
  {
    CNfoFile file;
    if (file.GetDetails(details,strResult.c_str()))
      return true;
  }
  return false;
}

