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
#include "VideoDatabase.h"
#include "addons/Scraper.h"
#include "NfoFile.h"
#include "VideoInfoDownloader.h"
#include "XBDateTime.h"

class CRegExp;

namespace VIDEO
{
  typedef struct SScanSettings
  {
    SScanSettings() { parent_name = parent_name_root = noupdate = exclude = false; recurse = 1;}
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
    bool exclude;           /* exclude this path from scraping */
  } SScanSettings;

  typedef struct SEpisode
  {
    CStdString strPath;
    CStdString strTitle;
    int iSeason;
    int iEpisode;
    bool isFolder;
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

  /*! \brief return values from the information lookup functions
   */
  enum INFO_RET { INFO_CANCELLED,
                  INFO_ERROR,
                  INFO_NOT_NEEDED,
                  INFO_HAVE_ALREADY,
                  INFO_NOT_FOUND,
                  INFO_ADDED };

  class CVideoInfoScanner : CThread
  {
  public:
    CVideoInfoScanner();
    virtual ~CVideoInfoScanner();

    /*! \brief Scan a folder using the background scanner
     \param strDirectory path to scan
     \param scanAll whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned.) Defaults to false.
     */
    void Start(const CStdString& strDirectory, bool scanAll = false);
    bool IsScanning();
    void Stop();
    void SetObserver(IVideoInfoScannerObserver* pObserver);

    /*! \brief Add an item to the database.
     \param pItem item to add to the database.
     \param content content type of the item.
     \param videoFolder whether the video is represented by a folder (single movie per folder). Defaults to false.
     \param idShow database id of the tvshow if we're adding an episode.  Defaults to -1.
     \return database id of the added item, or -1 on failure.
     */
    long AddVideo(CFileItem *pItem, const CONTENT_TYPE &content, bool videoFolder = false, int idShow = -1);

    /*! \brief Retrieve information for a list of items and add them to the database.
     \param items list of items to retrieve info for.
     \param bDirNames whether we should use folder or file names for lookups.
     \param content type of content to retrieve.
     \param useLocal should local data (.nfo and art) be used. Defaults to true.
     \param pURL an optional URL to use to retrieve online info.  Defaults to NULL.
     \param fetchEpisodes whether we are fetching episodes with shows. Defaults to true.
     \param pDlgProgress progress dialog to update and check for cancellation during processing.  Defaults to NULL.
     \return true if we successfully found information for some items, false otherwise
     */
    bool RetrieveVideoInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal = true, CScraperUrl *pURL = NULL, bool fetchEpisodes = true, CGUIDialogProgress* pDlgProgress = NULL);

    static void ApplyThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);
    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ADDON::ScraperPtr& scraper, CScraperUrl& scrUrl);

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
    bool DoScan(const CStdString& strDirectory);

    INFO_RET RetrieveInfoForTvShow(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, bool fetchEpisodes, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForMovie(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForMusicVideo(CFileItemPtr pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForEpisodes(CFileItemPtr item, long showID, const ADDON::ScraperPtr &scraper, bool useLocal, CGUIDialogProgress *progress = NULL);

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
     \return >0 on success, <0 on failure (cancellation), and 0 on no info found
     */
    int FindVideo(const CStdString &videoName, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    /*! \brief Retrieve detailed information for an item from an online source, optionally supplemented with local data
     TODO: sort out some better return codes.
     \param pItem item to retrieve online details for.
     \param url URL to use to retrieve online details.
     \param scraper Scraper that handles parsing the online data.
     \param nfoFile if set, we override the online data with the locally supplied data. Defaults to NULL.
     \param pDialog progress dialog to update and check for cancellation during processing. Defaults to NULL.
     \return true if information is found, false if an error occurred, the lookup was cancelled, or no information was found.
     */
    bool GetDetails(CFileItem *pItem, CScraperUrl &url, const ADDON::ScraperPtr &scraper, CNfoFile *nfoFile=NULL, CGUIDialogProgress* pDialog=NULL);

    /*! \brief Retrieve any artwork associated with an item
     \param pItem item to add to the database.
     \param content content type of the item.
     \param bApplyToDir whether we should apply any thumbs to a folder.  Defaults to false.
     \param useLocal whether we should use local thumbs. Defaults to true.
     \param pDialog progress dialog to update during processing. Defaults to NULL.
     */
    void GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir=false, bool useLocal=true, CGUIDialogProgress* pDialog = NULL);

    /*! \brief Extract episode and season numbers from a processed regexp
     \param reg Regular expression object with at least 2 matches
     \param episodeInfo Episode information to fill in.
     \return true on success (2 matches), false on failure (fewer than 2 matches)
     */
    bool GetEpisodeAndSeasonFromRegExp(CRegExp &reg, SEpisode &episodeInfo);

    /*! \brief Extract episode air-date from a processed regexp
     \param reg Regular expression object with at least 3 matches
     \param episodeInfo Episode information to fill in.
     \return true on success (3 matches), false on failure (fewer than 3 matches)
     */
    bool GetAirDateFromRegExp(CRegExp &reg, SEpisode &episodeInfo);

    void FetchActorThumbs(const std::vector<SActorInfo>& actors, const CStdString& strPath);
    static int GetPathHash(const CFileItemList &items, CStdString &hash);

    /*! \brief Retrieve a "fast" hash of the given directory (if available)
     Performs a stat() on the directory, and uses modified time to create a "fast"
     hash of the folder. If no modified time is available, the create time is used,
     and if neither are available, an empty hash is returned.
     \param directory folder to hash
     \return the hash of the folder of the form "fast<datetime>"
     */
    CStdString GetFastHash(const CStdString &directory) const;

    /*! \brief Decide whether a folder listing could use the "fast" hash
     Fast hashing can be done whenever the folder contains no scannable subfolders, as the
     fast hash technique uses modified time to determine when folder content changes, which
     is generally not propogated up the directory tree.
     \param items the directory listing
     \return true if this directory listing can be fast hashed, false otherwise
     */
    bool CanFastHash(const CFileItemList &items) const;

    /*! \brief Download an image file and apply the image to a folder if necessary
     \param url URL of the image.
     \param destination File to save the image as
     \param asThumb whether we need to download as a thumbnail or as a full image. Defaults to true
     \param progress progressbar to update - defaults to NULL
     \param directory directory that this thumbnail should be applied to. Defaults to empty
     */
    void DownloadImage(const CStdString &url, const CStdString &destination, bool asThumb = true, CGUIDialogProgress *dialog = NULL);

    /*! \brief Process a series folder, filling in episode details and adding them to the database.
     TODO: Ideally we would return INFO_HAVE_ALREADY if we don't have to update any episodes
     and we should return INFO_NOT_FOUND only if no information is found for any of
     the episodes. INFO_ADDED then indicates we've added one or more episodes.
     \param files the episode files to process.
     \param scraper scraper to use for finding online info
     \param idShow the database id of the show.
     \param strShowTitle the title of the show.
     \param pDlgProcess progress dialog to update during processing.  Defaults to NULL.
     \return INFO_ERROR on failure, INFO_CANCELLED on cancellation,
     INFO_NOT_FOUND if an episode isn't found, or INFO_ADDED if all episodes are added.
     */
    INFO_RET OnProcessSeriesFolder(EPISODES& files, const ADDON::ScraperPtr &scraper, bool useLocal, int idShow, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress = NULL);

    void EnumerateSeriesFolder(CFileItem* item, EPISODES& episodeList);
    bool EnumerateEpisodeItem(const CFileItemPtr item, EPISODES& episodeList);
    bool ProcessItemByVideoInfoTag(const CFileItemPtr item, EPISODES &episodeList);

    CStdString GetnfoFile(CFileItem *item, bool bGrabAny=false) const;

    /*! \brief Retrieve the parent folder of an item, accounting for stacks and files in rars.
     \param item a media item.
     \return the folder that contains the item.
     */
    CStdString GetParentDir(const CFileItem &item) const;

    IVideoInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bCanInterrupt;
    bool m_bClean;
    bool m_scanAll;
    CStdString m_strStartDir;
    CVideoDatabase m_database;
    std::set<CStdString> m_pathsToScan;
    std::set<CStdString> m_pathsToCount;
    std::set<int> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}

