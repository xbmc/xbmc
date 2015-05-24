/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoInfoDownloader.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogOK.h"
#include "ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace std;
using namespace VIDEO;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

CVideoInfoDownloader::CVideoInfoDownloader(const ADDON::ScraperPtr &scraper) :
  CThread("VideoInfoDownloader"), m_state(DO_NOTHING), m_found(0), m_info(scraper)
{
  m_http = new XFILE::CCurlFile;
}

CVideoInfoDownloader::~CVideoInfoDownloader()
{
  delete m_http;
}

// return value: 0 = we failed, -1 = we failed and reported an error, 1 = success
int CVideoInfoDownloader::InternalFindMovie(const std::string &strMovie,
                                            MOVIELIST& movielist,
                                            bool cleanChars /* = true */)
{
  try
  {
    movielist = m_info->FindMovie(*m_http, strMovie, cleanChars);
  }
  catch (const ADDON::CScraperError &sce)
  {
    ShowErrorDialog(sce);
    return sce.FAborted() ? 0 : -1;
  }
  return 1;  // success
}

void CVideoInfoDownloader::ShowErrorDialog(const ADDON::CScraperError &sce)
{
  if (!sce.Title().empty())
  {
    CGUIDialogOK *pdlg = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    pdlg->SetHeading(sce.Title());
    pdlg->SetLine(0, sce.Message());
    CApplicationMessenger::Get().DoModal(pdlg, WINDOW_DIALOG_OK);
  }
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

int CVideoInfoDownloader::FindMovie(const std::string &strMovie,
                                    MOVIELIST& movieList,
                                    CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::FindMovie(%s)", strMovie.c_str());

  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_strMovie = strMovie;
    m_found = 0;
    if (IsRunning())
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
    m_movieList.swap(movieList);
    int found=m_found;
    CloseThread();
    return found;
  }

  // unthreaded
  int success = InternalFindMovie(strMovie, movieList);
  // NOTE: this might be improved by rescraping if the match quality isn't high?
  if (success == 1 && movieList.empty())
  { // no results. try without cleaning chars like '.' and '_'
    success = InternalFindMovie(strMovie, movieList, false);
  }
  return success;
}

bool CVideoInfoDownloader::GetArtwork(CVideoInfoTag &details)
{
  return m_info->GetArtwork(*m_http, details);
}

bool CVideoInfoDownloader::GetDetails(const CScraperUrl &url,
                                      CVideoInfoTag &movieDetails,
                                      CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = 0;
    if (IsRunning())
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
    return m_info->GetVideoDetails(*m_http, url, true/*fMovie*/, movieDetails);
}

bool CVideoInfoDownloader::GetEpisodeDetails(const CScraperUrl &url,
                                             CVideoInfoTag &movieDetails,
                                             CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_DETAILS;
    m_found = 0;
    if (IsRunning())
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
    return m_info->GetVideoDetails(*m_http, url, false/*fMovie*/, movieDetails);
}

bool CVideoInfoDownloader::GetEpisodeList(const CScraperUrl& url,
                                          EPISODELIST& movieDetails,
                                          CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_episode = movieDetails;

  // fill in the defaults
  movieDetails.clear();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_LIST;
    m_found = 0;
    if (IsRunning())
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
    return !(movieDetails = m_info->GetEpisodeList(*m_http, url)).empty();
}

void CVideoInfoDownloader::CloseThread()
{
  m_http->Cancel();
  StopThread();
  m_http->Reset();
  m_state = DO_NOTHING;
  m_found = 0;
}

