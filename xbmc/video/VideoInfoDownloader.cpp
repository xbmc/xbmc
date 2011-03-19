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

#include "VideoInfoDownloader.h"
#include "Util.h"
#include "utils/HTMLUtil.h"
#include "utils/XMLUtils.h"
#include "utils/RegExp.h"
#include "utils/ScraperParser.h"
#include "NfoFile.h"
#include "dialogs/GUIDialogProgress.h"
#include "utils/fstrcmp.h"
#include "dialogs/GUIDialogOK.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace HTML;
using namespace std;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CVideoInfoDownloader::CVideoInfoDownloader(const ADDON::ScraperPtr &scraper)
{
  m_info = scraper;
}

CVideoInfoDownloader::~CVideoInfoDownloader()
{
}

int CVideoInfoDownloader::InternalFindMovie(const CStdString &strMovie,
                                            MOVIELIST& movielist,
                                            bool& sortMovieList,
                                            bool cleanChars /* = true */)
{
  movielist.clear();

  CScraperUrl scrURL;

  CStdString strName = strMovie;
  CStdString movieTitle, movieTitleAndYear, movieYear;
  CUtil::CleanString(strName, movieTitle, movieTitleAndYear, movieYear, true, cleanChars);

  movieTitle.ToLower();

  if (m_info->Content() == CONTENT_MUSICVIDEOS)
    movieTitle.Replace("-"," ");

  CLog::Log(LOGDEBUG, "%s: Searching for '%s' using %s scraper (path: '%s', content: '%s', version: '%s')",
    __FUNCTION__, movieTitle.c_str(), m_info->Name().c_str(), m_info->Path().c_str(),
    ADDON::TranslateContent(m_info->Content()).c_str(), m_info->Version().c_str());

  if (m_info->GetParser().HasFunction("CreateSearchUrl"))
  {
    GetURL(strMovie, movieTitle, movieYear, scrURL);
  }
  else if (m_info->Content() == CONTENT_MUSICVIDEOS)
  {
    if (!m_info->GetParser().HasFunction("FileNameScrape"))
      return 0;

    CScraperUrl scrURL("filenamescrape");
    URIUtils::RemoveExtension(strName);
    scrURL.strTitle = strName;
    movielist.push_back(scrURL);
    return 1;
  }
  if (scrURL.m_xml.IsEmpty())
    return 0;

  vector<CStdString> extras;
  extras.push_back(scrURL.m_url[0].m_url);

  vector<CStdString> xml = m_info->Run("GetSearchResults",scrURL,m_http,&extras);

  bool haveValidResults = false;
  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      continue;  // might have more valid results later
    }

    if (stricmp(doc.RootElement()->Value(),"error")==0)
    {
      ShowErrorDialog(doc.RootElement());
      return -1; // scraper has reported an error
    }

    TiXmlHandle docHandle( &doc );
    TiXmlElement *movie = docHandle.FirstChild("results").Element();
    if (!movie)
      continue;

    haveValidResults = true;

    movie = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
    while (movie)
    {
      // is our result already sorted correctly when handed over from scraper? if so, do not let xbmc sort it
      if (sortMovieList)
      {
        TiXmlElement* results = docHandle.FirstChild("results").Element();
        if (results)
        {
          CStdString szSorted = results->Attribute("sorted");
          sortMovieList = (szSorted.CompareNoCase("yes") != 0);
        }
      }

      TiXmlNode *title = movie->FirstChild("title");
      TiXmlElement *link = movie->FirstChildElement("url");
      TiXmlNode *year = movie->FirstChild("year");
      TiXmlNode* id = movie->FirstChild("id");
      TiXmlNode* language = movie->FirstChild("language");
      if (title && title->FirstChild() && link && link->FirstChild())
      {
        CScraperUrl url;
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
        // filter for dupes from naughty scrapers
        MOVIELIST::iterator iter=movielist.begin();
        while (iter != movielist.end())
        {
          if (iter->m_url[0].m_url.Equals(url.m_url[0].m_url) &&
              iter->strTitle.Equals(url.strTitle))
            break;
          ++iter;
        }
        if (iter == movielist.end())
          movielist.push_back(url);
      }
      movie = movie->NextSiblingElement();
    }
  }
  return haveValidResults ? 1 : 0;
}

bool CVideoInfoDownloader::RelevanceSortFunction(const CScraperUrl &left,
                                                 const CScraperUrl &right)
{
  return left.relevance > right.relevance;
}

void CVideoInfoDownloader::ShowErrorDialog(const TiXmlElement* element)
{
  const TiXmlElement* title = element->FirstChildElement("title");
  CStdString strTitle;
  if (title && title->FirstChild() && title->FirstChild()->Value())
    strTitle = title->FirstChild()->Value();
  const TiXmlElement* message = element->FirstChildElement("message");
  CStdString strMessage;
  if (message && message->FirstChild() && message->FirstChild()->Value())
    strMessage = message->FirstChild()->Value();
  CGUIDialogOK* dialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  dialog->SetHeading(strTitle);
  dialog->SetLine(0,strMessage);
  g_application.getApplicationMessenger().DoModal(dialog,WINDOW_DIALOG_OK);
}

bool CVideoInfoDownloader::InternalGetEpisodeList(const CScraperUrl& url,
                                                  EPISODELIST& details)
{
  EPISODELIST temp;
  vector<CStdString> extras;
  if (url.m_url.empty())
    return false;
  extras.push_back(url.m_url[0].m_url);
  vector<CStdString> xml = m_info->Run("GetEpisodeList",url,m_http,&extras);

  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str());
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      return false;
    }

    TiXmlHandle docHandle( &doc );
    TiXmlElement *movie = docHandle.FirstChild( "episodeguide" ).FirstChild( "episode" ).Element();

    while (movie)
    {
      TiXmlNode *title = movie->FirstChild("title");
      TiXmlElement *link = movie->FirstChildElement("url");
      TiXmlNode *epnum = movie->FirstChild("epnum");
      TiXmlNode *season = movie->FirstChild("season");
      TiXmlNode* id = movie->FirstChild("id");
      TiXmlNode *aired = movie->FirstChild("aired");
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
        EPISODE newEpisode;
        newEpisode.key = key;
        newEpisode.cDate.SetValid(FALSE);
        if (aired && aired->FirstChild())
        {
          const char *dateStr = aired->FirstChild()->Value();
          // date must be the format of yyyy-mm-dd
          if (strlen(dateStr)==10)
          {
            char year[5];
            char month[3];
            memcpy(year,dateStr,4);
            year[4] = '\0';
            memcpy(month,dateStr+5,2);
            month[2] = '\0';
            newEpisode.cDate.SetDate(atoi(year),atoi(month),atoi(dateStr+8));
          }
        }
        newEpisode.cScraperUrl = url2;
        temp.push_back(newEpisode);
      }
      movie = movie->NextSiblingElement();
    }
  }

  // find minimum in each season
  map<int,int> min;
  for (EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter )
  {
    if ((signed int) min.size() == (iter->key.first -1))
      min.insert(iter->key);
    else if (iter->key.second < min[iter->key.first])
      min[iter->key.first] = iter->key.second;
  }
  // correct episode numbers
  for (EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter )
  {
    int episode=iter->key.second - min[iter->key.first];
    if (min[iter->key.first] > 0)
      episode++;
    pair<int,int> key(iter->key.first,episode);
    EPISODE newEpisode;
    newEpisode.key = key;
    newEpisode.cDate = iter->cDate;
    newEpisode.cScraperUrl = iter->cScraperUrl;
    details.push_back(newEpisode);
  }

  return true;
}

bool CVideoInfoDownloader::InternalGetDetails(const CScraperUrl& url,
                                              CVideoInfoTag& movieDetails,
                                              const CStdString& strFunction)
{
  if (url.m_xml.Equals("filenamescrape"))
    return ScrapeFilename(movieDetails.m_strFileNameAndPath,movieDetails);

  vector<CStdString> extras;
  extras.push_back(url.strId);
  extras.push_back(url.m_url[0].m_url);
  vector<CStdString> xml = m_info->Run(strFunction,url,m_http,&extras);
  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      return false;
    }

    ParseDetails(doc, movieDetails);
  }

  return true;
}

bool CVideoInfoDownloader::ParseDetails(TiXmlDocument &doc,
                                        CVideoInfoTag &movieDetails)
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

  return true;
}

void CVideoInfoDownloader::RemoveAllAfter(char* szMovie, const char* szSearch)
{
  char* pPtr = strstr(szMovie, szSearch);
  if (pPtr) *pPtr = 0;
}

void CVideoInfoDownloader::GetURL(const CStdString &movieFile,
                                  const CStdString &movieName,
                                  const CStdString &movieYear,
                                  CScraperUrl& scrURL)
{
  // convert to the encoding requested by the parser
  vector<CStdString> extras;
  extras.push_back(movieName);
  g_charsetConverter.utf8To(m_info->GetParser().GetSearchStringEncoding(), movieName, extras[0]);
  CURL::Encode(extras[0]);
  if (!movieYear.IsEmpty())
    extras.push_back(movieYear);

  scrURL.Clear();
  vector<CStdString> xml = m_info->Run("CreateSearchUrl",scrURL,m_http,&extras);
  if (!xml.empty())
    scrURL.ParseString(xml[0]);
}

// threaded functions
void CVideoInfoDownloader::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = 0;
  if (m_state == FIND_MOVIE)
  {
    if (!(m_found=FindMovie(m_strMovie, m_movieList)))
      CLog::Log(LOGERROR, "%s: Error looking up item %s", __FUNCTION__, m_strMovie.c_str());
    m_state = DO_NOTHING;
    return;
  }

  if (m_url.m_url.empty())
  {
    // empty url when it's not supposed to be..
    // this might happen if the previously scraped item was removed from the site (see ticket #10537)
    CLog::Log(LOGERROR, "%s: Error getting details for %s due to an empty url", __FUNCTION__, m_strMovie.c_str());
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "%s: Error getting details from %s", __FUNCTION__,m_url.m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_DETAILS)
  {
    if (!GetEpisodeDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "%s: Error getting episode details from %s", __FUNCTION__, m_url.m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_LIST)
  {
    if (!GetEpisodeList(m_url, m_episode))
      CLog::Log(LOGERROR, "%s: Error getting episode list from %s", __FUNCTION__, m_url.m_url[0].m_url.c_str());
  }
  m_found = 1;
  m_state = DO_NOTHING;
}

int CVideoInfoDownloader::FindMovie(const CStdString &strMovie,
                                    MOVIELIST& movieList,
                                    CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::FindMovie(%s)", strMovie.c_str());

  // load our scraper xml
  if (!m_info->Load())
    return 0;

  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_strMovie = strMovie;
    m_found = 0;
    if (ThreadHandle())
      StopThread();
    Create();
    while (m_state != DO_NOTHING)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return 0;
      }
      Sleep(1);
    }
    // transfer to our movielist
    movieList.clear();
    for (unsigned int i=0; i < m_movieList.size(); i++)
      movieList.push_back(m_movieList[i]);
    m_movieList.clear();
    int found=m_found;
    CloseThread();
    return found;
  }

  // unthreaded
  bool sortList = true;
  int success = InternalFindMovie(strMovie, movieList, sortList);
  // NOTE: this might be improved by rescraping if the match quality isn't high?
  if (success && !movieList.size())
  { // no results. try without cleaning chars like '.' and '_'
    success = InternalFindMovie(strMovie, movieList, sortList, false);
  }
  // sort our movie list by fuzzy match
  if (sortList)
    std::sort(movieList.begin(), movieList.end(), RelevanceSortFunction);
  return success;
}

bool CVideoInfoDownloader::GetDetails(const CScraperUrl &url,
                                      CVideoInfoTag &movieDetails,
                                      CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;
  // load our scraper xml
  if (!m_info->Load())
    return false;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = 0;
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

bool CVideoInfoDownloader::GetEpisodeDetails(const CScraperUrl &url,
                                             CVideoInfoTag &movieDetails,
                                             CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // load our scraper xml
  if (!m_info->Load())
    return false;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_DETAILS;
    m_found = 0;
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

bool CVideoInfoDownloader::GetEpisodeList(const CScraperUrl& url,
                                          EPISODELIST& movieDetails,
                                          CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_episode = movieDetails;

  // load our scraper xml
  if (!m_info->Load())
    return false;

  // fill in the defaults
  movieDetails.clear();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_LIST;
    m_found = 0;
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

void CVideoInfoDownloader::CloseThread()
{
  m_http.Cancel();
  StopThread();
  m_http.Reset();
  m_state = DO_NOTHING;
  m_found = 0;
}

bool CVideoInfoDownloader::ScrapeFilename(const CStdString& strFileName,
                                          CVideoInfoTag& details)
{
  CScraperUrl url;
  vector<CStdString> extras;
  extras.push_back(strFileName);
  URIUtils::RemoveExtension(extras[0]);
  extras[0].Replace("_"," ");
  vector<CStdString> result = m_info->Run("FileNameScrape",url,m_http,&extras);
  if (!result.empty())
  {
    CLog::Log(LOGDEBUG,"scraper: FileNameScrape returned %s", result[0].c_str());
    TiXmlDocument doc;
    doc.Parse(result[0].c_str());
    if (doc.RootElement())
    {
      CNfoFile file;
      if (file.GetDetails(details,result[0].c_str()))
        return true;
    }
  }

  return false;
}

