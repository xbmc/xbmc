/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoScanner.h"
#include "VideoDatabase.h"
#include "addons/Scraper.h"
#include "guilib/GUIListItem.h"

#include <set>
#include <string>
#include <vector>

class CRegExp;
class CFileItem;
class CFileItemList;

namespace KODI::VIDEO
{
  class IVideoInfoTagLoader;

  typedef struct SScanSettings
  {
    SScanSettings()
    {
      parent_name = false;
      parent_name_root = false;
      noupdate = false;
      exclude = false;
      m_allExtAudio = false;
      recurse = 1;
    }
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
    bool exclude;           /* exclude this path from scraping */
    bool m_allExtAudio; /* treat all audio files in video directory as external tracks */
  } SScanSettings;

  class CVideoInfoScanner : public CInfoScanner
  {
  public:
    CVideoInfoScanner();
    ~CVideoInfoScanner() override;

    /*! \brief Scan a folder using the background scanner
     \param strDirectory path to scan
     \param scanAll whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned.) Defaults to false.
     */
    void Start(const std::string& strDirectory, bool scanAll = false);
    void Stop();

    /*! \brief Add an item to the database.
     \param pItem item to add to the database.
     \param content content type of the item.
     \param videoFolder whether the video is represented by a folder (single movie per folder). Defaults to false.
     \param useLocal whether to use local information for artwork etc.
     \param showInfo pointer to CVideoInfoTag details for the show if this is an episode. Defaults to NULL.
     \param libraryImport Whether this call belongs to a full library import or not. Defaults to false.
     \return database id of the added item, or -1 on failure.
     */
    long AddVideo(CFileItem *pItem, const CONTENT_TYPE &content, bool videoFolder = false, bool useLocal = true, const CVideoInfoTag *showInfo = NULL, bool libraryImport = false);

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

    static void ApplyThumbToFolder(const std::string &folder, const std::string &imdbThumb);
    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);

    /*! \brief Retrieve any artwork associated with an item
     \param pItem item to find artwork for.
     \param content content type of the item.
     \param bApplyToDir whether we should apply any thumbs to a folder.  Defaults to false.
     \param useLocal whether we should use local thumbs. Defaults to true.
     \param actorArtPath the path to search for actor thumbs. Defaults to empty.
     */
    void GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir=false, bool useLocal=true, const std::string &actorArtPath = "");

    /*! \brief Get season thumbs for a tvshow.
     All seasons (regardless of whether the user has episodes) are added to the art map.
     \param show     tvshow info tag
     \param art      artwork map to which season thumbs are added.
     \param useLocal whether to use local thumbs, defaults to true
     */
    static void GetSeasonThumbs(const CVideoInfoTag &show, std::map<int, std::map<std::string, std::string> > &art, const std::vector<std::string> &artTypes, bool useLocal = true);
    static std::string GetImage(const CScraperUrl::SUrlEntry &image, const std::string& itemPath);

    bool EnumerateEpisodeItem(const CFileItem *item, EPISODELIST& episodeList);

    static std::string GetMovieSetInfoFolder(const std::string& setTitle);

  protected:
    virtual void Process();
    bool DoScan(const std::string& strDirectory) override;

    INFO_RET RetrieveInfoForTvShow(CFileItem *pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, bool fetchEpisodes, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForMovie(CFileItem *pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForMusicVideo(CFileItem *pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);
    INFO_RET RetrieveInfoForEpisodes(CFileItem *item, long showID, const ADDON::ScraperPtr &scraper, bool useLocal, CGUIDialogProgress *progress = NULL);

    /*! \brief Update the progress bar with the heading and line and check for cancellation
     \param progress CGUIDialogProgress bar
     \param heading string id of heading
     \param line1   string to set for the first line
     \return true if the user has cancelled the scanner, false otherwise
     */
    bool ProgressCancelled(CGUIDialogProgress* progress, int heading, const std::string &line1);

    /*! \brief Find a url for the given video using the given scraper
     \param title title of the video to lookup
     \param year year of the video to lookup
     \param scraper scraper to use for the lookup
     \param url [out] returned url from the scraper
     \param progress CGUIDialogProgress bar
     \return >0 on success, <0 on failure (cancellation), and 0 on no info found
     */
    int FindVideo(const std::string &title, int year, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    /*! \brief Find a url for the given video using the given scraper
     \param item the video to lookup
     \param scraper scraper to use for the lookup
     \param url [out] returned url from the scraper
     \param progress CGUIDialogProgress bar
     \return >0 on success, <0 on failure (cancellation), and 0 on no info found
     */
    int FindVideoUsingTag(CFileItem& item, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    /*! \brief Retrieve detailed information for an item from an online source, optionally supplemented with local data
     @todo sort out some better return codes.
     \param pItem item to retrieve online details for.
     \param uniqueIDs Unique IDs for additional information for scrapers.
     \param url URL to use to retrieve online details.
     \param scraper Scraper that handles parsing the online data.
     \param nfoFile if set, we override the online data with the locally supplied data. Defaults to NULL.
     \param pDialog progress dialog to update and check for cancellation during processing. Defaults to NULL.
     \return true if information is found, false if an error occurred, the lookup was cancelled, or no information was found.
     */
    bool GetDetails(CFileItem* pItem,
                    const std::unordered_map<std::string, std::string>& uniqueIDs,
                    CScraperUrl& url,
                    const ADDON::ScraperPtr& scraper,
                    VIDEO::IVideoInfoTagLoader* nfoFile = nullptr,
                    CGUIDialogProgress* pDialog = nullptr);

    /*! \brief Extract episode and season numbers from a processed regexp
     \param reg Regular expression object with at least 2 matches
     \param episodeInfo Episode information to fill in.
     \param defaultSeason Season to use if not found in reg.
     \return true on success (2 matches), false on failure (fewer than 2 matches)
     */
    bool GetEpisodeAndSeasonFromRegExp(CRegExp &reg, EPISODE &episodeInfo, int defaultSeason);

    /*! \brief Extract episode air-date from a processed regexp
     \param reg Regular expression object with at least 3 matches
     \param episodeInfo Episode information to fill in.
     \return true on success (3 matches), false on failure (fewer than 3 matches)
     */
    bool GetAirDateFromRegExp(CRegExp &reg, EPISODE &episodeInfo);

    /*! \brief Extract episode title from a processed regexp
     \param reg Regular expression object with at least 1 match
     \param episodeInfo Episode information to fill in.
     \return true on success (1 match), false on failure (no matches)
     */
    bool GetEpisodeTitleFromRegExp(CRegExp& reg, EPISODE& episodeInfo);

    /*! \brief Fetch thumbs for actors
     Updates each actor with their thumb (local or online)
     \param actors - vector of SActorInfo
     \param strPath - path on filesystem to look for local thumbs
     */
    void FetchActorThumbs(std::vector<SActorInfo>& actors, const std::string& strPath);

    static int GetPathHash(const CFileItemList &items, std::string &hash);

    /*! \brief Retrieve a "fast" hash of the given directory (if available)
     Performs a stat() on the directory, and uses modified time to create a "fast"
     hash of the folder. If no modified time is available, the create time is used,
     and if neither are available, an empty hash is returned.
     In case exclude from scan expressions are present, the string array will be appended
     to the md5 hash to ensure we're doing a re-scan whenever the user modifies those.
     \param directory folder to hash
     \param excludes string array of exclude expressions
     \return the md5 hash of the folder"
     */
    std::string GetFastHash(const std::string &directory, const std::vector<std::string> &excludes) const;

    /*! \brief Retrieve a "fast" hash of the given directory recursively (if available)
     Performs a stat() on the directory, and uses modified time to create a "fast"
     hash of each folder. If no modified time is available, the create time is used,
     and if neither are available, an empty hash is returned.
     In case exclude from scan expressions are present, the string array will be appended
     to the md5 hash to ensure we're doing a re-scan whenever the user modifies those.
     \param directory folder to hash (recursively)
     \param excludes string array of exclude expressions
     \return the md5 hash of the folder
     */
    std::string GetRecursiveFastHash(const std::string &directory, const std::vector<std::string> &excludes) const;

    /*! \brief Decide whether a folder listing could use the "fast" hash
     Fast hashing can be done whenever the folder contains no scannable subfolders, as the
     fast hash technique uses modified time to determine when folder content changes, which
     is generally not propagated up the directory tree.
     \param items the directory listing
     \param excludes string array of exclude expressions
     \return true if this directory listing can be fast hashed, false otherwise
     */
    bool CanFastHash(const CFileItemList &items, const std::vector<std::string> &excludes) const;

    /*! \brief Process a series folder, filling in episode details and adding them to the database.
     @todo Ideally we would return INFO_HAVE_ALREADY if we don't have to update any episodes
     and we should return INFO_NOT_FOUND only if no information is found for any of
     the episodes. INFO_ADDED then indicates we've added one or more episodes.
     \param files the episode files to process.
     \param scraper scraper to use for finding online info
     \param showInfo information for the show.
     \param pDlgProcess progress dialog to update during processing.  Defaults to NULL.
     \return INFO_ERROR on failure, INFO_CANCELLED on cancellation,
     INFO_NOT_FOUND if an episode isn't found, or INFO_ADDED if all episodes are added.
     */
    INFO_RET OnProcessSeriesFolder(EPISODELIST& files, const ADDON::ScraperPtr &scraper, bool useLocal, const CVideoInfoTag& showInfo, CGUIDialogProgress* pDlgProgress = NULL);

    bool EnumerateSeriesFolder(CFileItem* item, EPISODELIST& episodeList);
    bool ProcessItemByVideoInfoTag(const CFileItem *item, EPISODELIST &episodeList);

    bool AddVideoExtras(CFileItemList& items, const CONTENT_TYPE& content, const std::string& path);
    bool ProcessVideoVersion(VideoDbContentType itemType, int dbId);

    bool m_bStop;
    bool m_scanAll;
    bool m_ignoreVideoVersions{false};
    bool m_ignoreVideoExtras{false};
    std::string m_strStartDir;
    CVideoDatabase m_database;
    std::set<std::string> m_pathsToCount;
    std::set<int> m_pathsToClean;

  private:
    static void AddLocalItemArtwork(CGUIListItem::ArtMap& itemArt,
      const std::vector<std::string>& wantedArtTypes, const std::string& itemPath,
      bool addAll, bool exactName);

    /*! \brief Retrieve the art type for an image from the given size.
     \param width the width of the image.
     \param height the height of the image.
     \return "poster" if the aspect ratio is at most 4:5, "banner" if the aspect ratio
             is at least 1:4, "thumb" otherwise.
     */
    static std::string GetArtTypeFromSize(unsigned int width, unsigned int height);
  };
  } // namespace KODI::VIDEO
