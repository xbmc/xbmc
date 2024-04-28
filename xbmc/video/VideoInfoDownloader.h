/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Episode.h"
#include "VideoInfoTag.h"
#include "addons/Scraper.h"
#include "threads/Thread.h"

#include <string>
#include <vector>

// forward declarations
class CXBMCTinyXML;
class CGUIDialogProgress;

namespace ADDON
{
class CScraperError;
}
namespace XFILE
{
class CurlFile;
}

typedef std::vector<CScraperUrl> MOVIELIST;

class CVideoInfoDownloader : public CThread
{
public:
  explicit CVideoInfoDownloader(const ADDON::ScraperPtr &scraper);
  ~CVideoInfoDownloader() override;

  // threaded lookup functions

  /*! \brief Do a search for matching media items (possibly asynchronously) with our scraper
   \param movieTitle title of the media item to look for
   \param movieYear year of the media item to look for (-1 if not known)
   \param movielist [out] list of results to fill. May be empty on success.
   \param pProgress progress bar to update as we go. If NULL we run on thread, if non-NULL we run off thread.
   \return 1 on success, -1 on a scraper-specific error, 0 on some other error
   */
  int FindMovie(const std::string& movieTitle, int movieYear, MOVIELIST& movielist, CGUIDialogProgress *pProgress = NULL);

  /*! \brief Fetch art URLs for an item with our scraper
   \param details the video info tag structure to fill with art.
   \return true on success, false on failure.
   */
  bool GetArtwork(CVideoInfoTag &details);

  bool GetDetails(const std::unordered_map<std::string, std::string>& uniqueIDs,
                  const CScraperUrl& url,
                  CVideoInfoTag& movieDetails,
                  CGUIDialogProgress* pProgress = NULL);
  bool GetEpisodeDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeList(const CScraperUrl& url,
                      KODI::VIDEO::EPISODELIST& details,
                      CGUIDialogProgress* pProgress = NULL);

  static void ShowErrorDialog(const ADDON::CScraperError &sce);

protected:
  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_MOVIE = 1,
                      GET_DETAILS = 2,
                      GET_EPISODE_LIST = 3,
                      GET_EPISODE_DETAILS = 4 };

  XFILE::CCurlFile*   m_http;
  std::string         m_movieTitle;
  int                 m_movieYear;
  std::unordered_map<std::string, std::string> m_uniqueIDs;
  MOVIELIST           m_movieList;
  CVideoInfoTag       m_movieDetails;
  CScraperUrl         m_url;
  KODI::VIDEO::EPISODELIST m_episode;
  LOOKUP_STATE m_state = DO_NOTHING;
  int m_found = 0;
  ADDON::ScraperPtr   m_info;

  // threaded stuff
  void Process() override;
  void CloseThread();

  int InternalFindMovie(const std::string& movieTitle, int movieYear, MOVIELIST& movielist, bool cleanChars = true);
};

