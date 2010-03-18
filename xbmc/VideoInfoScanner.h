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
#include "utils/Thread.h"
#include "VideoDatabase.h"
#include "addons/Scraper.h"
#include "NfoFile.h"
#include "IMDB.h"
#include "DateTime.h"

class CIMDB;

namespace VIDEO
{
  typedef struct SScanSettings
  {
    SScanSettings() { parent_name = parent_name_root = noupdate = exclude = false; recurse = -1;}
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
    bool exclude;           /* exclude this path from scraping */
  } SScanSettings;

  typedef struct SEpisode
  {
    CStdString strPath;
    int iSeason;
    int iEpisode;
    CDateTime cDate;
  } SEpisode;

  typedef std::vector<SEpisode> EPISODES;

  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, FETCHING_MOVIE_INFO, FETCHING_MUSICVIDEO_INFO, FETCHING_TVSHOW_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

  class IVideoInfoScannerObserver
  {
  public:
    virtual ~IVideoInfoScannerObserver() { }
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnSetCurrentProgress(int currentItem, int itemCount)=0;
    virtual void OnSetTitle(const CStdString& strTitle) = 0;
    virtual void OnFinished() = 0;
  };

  class CVideoInfoScanner : CThread, public IRunnable
  {
  public:
    CVideoInfoScanner();
    virtual ~CVideoInfoScanner();
    void Start(const CStdString& strDirectory, const ADDON::ScraperPtr& info, const SScanSettings& settings, bool bUpdateAll);
    bool IsScanning();
    void Stop();
    void SetObserver(IVideoInfoScannerObserver* pObserver);

    void EnumerateSeriesFolder(CFileItem* item, EPISODES& episodeList);
    bool ProcessItemNormal(CFileItemPtr item, EPISODES& episodeList, CStdString regexp);
    bool ProcessItemByDate(CFileItemPtr item, EPISODES& eipsodeList, CStdString regexp);
    long AddMovie(CFileItem *pItem, const CONTENT_TYPE &content, CVideoInfoTag &movieDetails, int idShow = -1);
    long AddMovieAndGetThumb(CFileItem *pItem, const CONTENT_TYPE &content, CVideoInfoTag &movieDetails, int idShow, bool bApplyToDir=false, bool bRefresh=false, CGUIDialogProgress* pDialog = NULL);
    bool OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, EPISODES& files, int idShow, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress = NULL);
    static CStdString GetnfoFile(CFileItem *item, bool bGrabAny=false);
    long GetIMDBDetails(CFileItem *pItem, CScraperUrl &url, const ADDON::ScraperPtr &scraper, bool bUseDirNames=false, CGUIDialogProgress* pDialog=NULL, bool bCombined=false, bool bRefresh=false);
    bool RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const ADDON::ScraperPtr &info, bool bRefresh=false, CScraperUrl *pURL=NULL, CGUIDialogProgress* pDlgProgress  = NULL, bool ignoreNfo=false);
    static void ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);
    static int GetPathHash(const CFileItemList &items, CStdString &hash);
    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ADDON::ScraperPtr& scraper, CScraperUrl& scrUrl);
    CIMDB m_IMDB;
    /*! \brief Fetch thumbs for seasons for a given show
     Fetches and caches local season thumbs of the form season##.tbn and season-all.tbn for the current show,
     and downloads online thumbs if they don't exist.
     \param idTvShow database id of the tvshow.
     \param folderToCheck folder to check for local thumbs, if other than the show folder.  Defaults to empty.
     \param download whether we should download thumbs that don't exist.  Defaults to true.
     \param overwrite whether to overwrite currently cached thumbs.  Defaults to false.
     */
    void FetchSeasonThumbs(int idTvShow, const CStdString &folderToCheck = "", bool download = true, bool overwrite = false);
  protected:
    virtual void Process();
    bool DoScan(const CStdString& strDirectory, SScanSettings settings);

    int RetreiveInfoForTvShow(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool bRefresh, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress, bool ignoreNfo);
    int RetreiveInfoForMovie(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool bRefresh, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress, bool ignoreNfo);
    int RetreiveInfoForMusicVideo(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool bRefresh, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress, bool ignoreNfo);

    /*! \brief Update the progress bar with the heading and line and check for cancellation
     \param progress CGUIDialogProgress bar
     \param heading string id of heading
     \param line1   string to set for the first line
     \return true if the user has cancelled the scanner, false otherwise
     */
    bool ProgressCancelled(CGUIDialogProgress* progress, int heading, const CStdString &line1);

    /*! \brief Find a url for the given video using the given scraper
     \param videoName name of the video to lookup
     \param scraper scraper to use for the lookup
     \param url [out] returned url from the scraper
     \param progress CGUIDialogProgress bar
     \return >0 on success, <0 on failure, and 0 on no info found
     */
    int FindVideo(const CStdString &videoName, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    virtual void Run();
    int CountFiles(const CStdString& strPath);
    void FetchActorThumbs(const std::vector<SActorInfo>& actors, const CStdString& strPath);
    void SetScraperInfo(const ADDON::ScraperPtr& info) { m_info = info; };

  protected:
    IVideoInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bUpdateAll;
    bool m_bCanInterrupt;
    bool m_bClean;
    CStdString m_strStartDir;
    CVideoDatabase m_database;
    ADDON::ScraperPtr m_info;
    std::map<CStdString,SScanSettings> m_pathsToScan;
    std::set<CStdString> m_pathsToCount;
    std::vector<int> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}

