#pragma once

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

#include "threads/Thread.h"
#include "VideoInfoTag.h"
#include "addons/Scraper.h"
#include "utils/Episode.h"
#include "XBDateTime.h"
#include "filesystem/FileCurl.h"

// forward declarations
class TiXmlDocument;
class CGUIDialogProgress;

typedef std::vector<CScraperUrl> MOVIELIST;

class CVideoInfoDownloader : public CThread
{
public:
  CVideoInfoDownloader(const ADDON::ScraperPtr &scraper);
  virtual ~CVideoInfoDownloader();

  // threaded lookup functions

  /*! \brief Do a search for matching media items (possibly asynchronously) with our scraper
   \param strMovie name of the media item to look for
   \param movielist [out] list of results to fill. May be empty on success.
   \param pProgress progress bar to update as we go. If NULL we run on thread, if non-NULL we run off thread.
   \return 1 on success, -1 on a scraper-specific error, 0 on some other error
   */
  int FindMovie(const CStdString& strMovie, MOVIELIST& movielist, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeList(const CScraperUrl& url, EPISODELIST& details, CGUIDialogProgress *pProgress = NULL);

  static void ShowErrorDialog(const TiXmlElement* element);
protected:
  void RemoveAllAfter(char* szMovie, const char* szSearch);
  int InternalFindMovie(const CStdString& strMovie, MOVIELIST& movielist, bool& sortMovieList, bool cleanChars = true);
  bool InternalGetDetails(const CScraperUrl& url, CVideoInfoTag& movieDetails, const CStdString& strFunction="GetDetails");
  bool InternalGetEpisodeList(const CScraperUrl& url, EPISODELIST& details);
  bool ParseDetails(TiXmlDocument &doc, CVideoInfoTag &movieDetails);
  void GetURL(const CStdString &movieFile, const CStdString &movieName, const CStdString &movieYear, CScraperUrl& strURL);
  bool ScrapeFilename(const CStdString& strFileName, CVideoInfoTag& details);

  static bool RelevanceSortFunction(const CScraperUrl& left, const CScraperUrl &right);

  XFILE::CFileCurl m_http;

  // threaded stuff
  void Process();
  void CloseThread();

  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_MOVIE = 1,
                      GET_DETAILS = 2,
                      GET_EPISODE_LIST = 3,
                      GET_EPISODE_DETAILS = 4 };
  CStdString        m_strMovie;
  MOVIELIST         m_movieList;
  CVideoInfoTag     m_movieDetails;
  CScraperUrl       m_url;
  EPISODELIST       m_episode;
  LOOKUP_STATE      m_state;
  int               m_found;
  ADDON::ScraperPtr m_info;
};

