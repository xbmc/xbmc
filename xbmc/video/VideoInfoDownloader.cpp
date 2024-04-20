/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoDownloader.h"

#include "dialogs/GUIDialogProgress.h"
#include "filesystem/CurlFile.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace std::chrono_literals;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

CVideoInfoDownloader::CVideoInfoDownloader(const ADDON::ScraperPtr& scraper)
  : CThread("VideoInfoDownloader"), m_info(scraper)
{
  m_http = new XFILE::CCurlFile;
}

CVideoInfoDownloader::~CVideoInfoDownloader()
{
  delete m_http;
}

// return value: 0 = we failed, -1 = we failed and reported an error, 1 = success
int CVideoInfoDownloader::InternalFindMovie(const std::string &movieTitle, int movieYear,
                                            MOVIELIST& movielist,
                                            bool cleanChars /* = true */)
{
  try
  {
    movielist = m_info->FindMovie(*m_http, movieTitle, movieYear, cleanChars);
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
    HELPERS::ShowOKDialogText(CVariant{ sce.Title() }, CVariant{ sce.Message() });
}

// threaded functions
void CVideoInfoDownloader::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = 0;
  if (m_state == FIND_MOVIE)
  {
    if (!(m_found=FindMovie(m_movieTitle, m_movieYear, m_movieList)))
      CLog::Log(LOGERROR, "{}: Error looking up item {} ({})", __FUNCTION__, m_movieTitle,
                m_movieYear);
    m_state = DO_NOTHING;
    return;
  }

  if (!m_url.HasUrls() && m_uniqueIDs.empty())
  {
    // empty url when it's not supposed to be..
    // this might happen if the previously scraped item was removed from the site (see ticket #10537)
    CLog::Log(LOGERROR, "{}: Error getting details for {} ({}) due to an empty url", __FUNCTION__,
              m_movieTitle, m_movieYear);
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_uniqueIDs, m_url, m_movieDetails))
      CLog::Log(LOGERROR, "{}: Error getting details from {}", __FUNCTION__,
                m_url.GetFirstThumbUrl());
  }
  else if (m_state == GET_EPISODE_DETAILS)
  {
    if (!GetEpisodeDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "{}: Error getting episode details from {}", __FUNCTION__,
                m_url.GetFirstThumbUrl());
  }
  else if (m_state == GET_EPISODE_LIST)
  {
    if (!GetEpisodeList(m_url, m_episode))
      CLog::Log(LOGERROR, "{}: Error getting episode list from {}", __FUNCTION__,
                m_url.GetFirstThumbUrl());
  }
  m_found = 1;
  m_state = DO_NOTHING;
}

int CVideoInfoDownloader::FindMovie(const std::string &movieTitle, int movieYear,
                                    MOVIELIST& movieList,
                                    CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::FindMovie({})", strMovie);

  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_movieTitle = movieTitle;
    m_movieYear = movieYear;
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
      CThread::Sleep(1ms);
    }
    // transfer to our movielist
    m_movieList.swap(movieList);
    int found=m_found;
    CloseThread();
    return found;
  }

  // unthreaded
  int success = InternalFindMovie(movieTitle, movieYear, movieList);
  // NOTE: this might be improved by rescraping if the match quality isn't high?
  if (success == 1 && movieList.empty())
  { // no results. try without cleaning chars like '.' and '_'
    success = InternalFindMovie(movieTitle, movieYear, movieList, false);
  }
  return success;
}

bool CVideoInfoDownloader::GetArtwork(CVideoInfoTag &details)
{
  return m_info->GetArtwork(*m_http, details);
}

bool CVideoInfoDownloader::GetDetails(const std::unordered_map<std::string, std::string>& uniqueIDs,
                                      const CScraperUrl& url,
                                      CVideoInfoTag& movieDetails,
                                      CGUIDialogProgress* pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails({})", url.m_strURL);
  m_url = url;
  m_uniqueIDs = uniqueIDs;
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
      CThread::Sleep(1ms);
    }
    movieDetails = m_movieDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return m_info->GetVideoDetails(*m_http, m_uniqueIDs, url, true /*fMovie*/, movieDetails);
}

bool CVideoInfoDownloader::GetEpisodeDetails(const CScraperUrl &url,
                                             CVideoInfoTag &movieDetails,
                                             CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails({})", url.m_strURL);
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
      CThread::Sleep(1ms);
    }
    movieDetails = m_movieDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return m_info->GetVideoDetails(*m_http, m_uniqueIDs, url, false /*fMovie*/, movieDetails);
}

bool CVideoInfoDownloader::GetEpisodeList(const CScraperUrl& url,
                                          VIDEO::EPISODELIST& movieDetails,
                                          CGUIDialogProgress* pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CVideoInfoDownloader::GetDetails({})", url.m_strURL);
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
      CThread::Sleep(1ms);
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

