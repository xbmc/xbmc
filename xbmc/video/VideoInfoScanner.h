/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "FileItemList.h"
#include "InfoScanner.h"
#include "VideoDatabase.h"
#include "VideoManagerTypes.h"
#include "XBDateTime.h"
#include "addons/Scraper.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "settings/VideoVersionsSettings.h"
#include "utils/RegExp.h"
#include "utils/StreamDetails.h"
#include "video/VideoInfoScannerArt.h"
#include "video/VideoInfoScannerExtras.h"
#include "video/VideoInfoScannerLookups.h"
#include "video/VideoInfoScannerWalk.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class CAdvancedSettings;
class CRegExp;
class CFileItem;
class CFileItemList;
class CVideoInfoDownloader;

namespace KODI::VIDEO
{
  class IVideoInfoTagLoader;
  class ISetInfoTagLoader;

  class CVideoInfoScanner : public CInfoScanner
  {
  public:
    CVideoInfoScanner();
    ~CVideoInfoScanner() override;

    /*! \brief Retrieve any artwork associated with an item
     \sa CVideoInfoScannerArt::GetArtwork
     */
    void GetArtwork(CFileItem* pItem,
                    ADDON::ContentType content,
                    bool bApplyToDir = false,
                    bool useLocal = true,
                    const std::string& actorArtPath = "",
                    CVideoInfoScannerArt::UseRemoteArtWithLocalScraper useRemoteArt =
                        CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::YES,
                    const CFileItem* mediaItem = nullptr) const
    {
      m_art.GetArtwork(pItem, content, bApplyToDir, useLocal, actorArtPath, useRemoteArt,
                       mediaItem);
    }

    /*! \brief Scan a folder using the background scanner
     \param strDirectory path to scan
     \param scanAll whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned.) Defaults to false.
     */
    void Start(const std::string& strDirectory, bool scanAll = false);
    void Stop();

    /*! \brief Add a set to the database.
     \param set CSetInfoTag to add to the database.
     \return true if successful, false otherwise.
     */
    bool AddSet(const CSetInfoTag& set);

    /*! \brief Add an item to the database.
     \param pItem item to add to the database.
     \param scraper scraper used for the lookup.
     \param videoFolder whether the video is represented by a folder (single movie per folder). Defaults to false.
     \param useLocal whether to use local information for artwork etc.
     \param showInfo pointer to CVideoInfoTag details for the show if this is an episode. Defaults to nullptr.
     \param libraryImport Whether this call belongs to a full library import or not. Defaults to false.
     \param contentOverride content type of the item. Defaults to CONTENT_NONE (which is ignored).
     \return database id of the added item, or -1 on failure.
     */
    long AddVideo(CFileItem* pItem,
                  const ADDON::ScraperPtr& scraper,
                  bool videoFolder = false,
                  bool useLocal = true,
                  const CVideoInfoTag* showInfo = nullptr,
                  bool libraryImport = false,
                  ADDON::ContentType contentOverride = ADDON::ContentType::NONE);

    /*! \brief Retrieve information for a list of items and add them to the database.
     \param items list of items to retrieve info for.
     \param bDirNames whether we should use folder or file names for lookups.
     \param content type of content to retrieve.
     \param useLocal should local data (.nfo and art) be used. Defaults to true.
     \param pURL an optional URL to use to retrieve online info.  Defaults to nullptr.
     \param fetchEpisodes whether we are fetching episodes with shows. Defaults to true.
     \param pDlgProgress progress dialog to update and check for cancellation during processing.  Defaults to nullptr.
     \return true if we successfully found information for some items, false otherwise
     */
    bool RetrieveVideoInfo(const CFileItemList& items,
                           bool bDirNames,
                           ADDON::ContentType content,
                           bool useLocal = true,
                           CScraperUrl* pURL = nullptr,
                           bool fetchEpisodes = true,
                           CGUIDialogProgress* pDlgProgress = nullptr);

    /*! \brief Leave adding the extras of the movies RetrieveVideoInfo() adds to AddMovieExtras().
     A refresh adds a movie again before moving its old versions and extras to it, and they must
     be its own for those already there to be seen.
     */
    void DeferMovieExtras() { m_deferMovieExtras = true; }

    /*! \brief Look the videos up with these scrapers, as CVideoDatabase::GetScraperForPath()
     cached them, so that the details already read with them are read from their cache.
     */
    void UseScrapers(const CVideoDatabase::ScraperCache& scrapers)
    {
      m_scraperCache.insert(scrapers.begin(), scrapers.end());
    }

    /*! \brief Look the movies or tv shows up by these ids with a python scraper, rather than search
     for them.
     */
    void LookUpBy(const ADDON::CScraper::UniqueIDs& uniqueIDs) { m_uniqueIDs = uniqueIDs; }

    /*! \brief Add the extras of a refreshed movie: those its discs name, and those of the extras
     folders beside it, which a scan finds as it lists the movie's folders.
     \param dbId the movie, once its versions and extras are its own
     \param useFolderNames whether the movie's folder names it (the scraper's "movies are in
     separate folders"), without which it has no extras folders
     */
    void AddMovieExtras(int dbId, bool useFolderNames);

    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);

    /*! \brief Update the set information from a SET.NFO in the Movie Set Information Folder
     Gets set details from the VideoInfoTag of a movie
     \param tag     info tag
     */
    static bool UpdateSetInTag(CVideoInfoTag& tag);

    static std::string GetMovieSetInfoFolder(const std::string& setTitle);

  protected:
    virtual void Process();
    std::pair<ScanComplete, ContentFound> DoScan(const std::string& strDirectory) override;

    //! \brief The name of the video source holding \p folder, else the folder's own name
    static std::string SourceName(const std::string& folder);

    struct ItemLookups;
    //! \brief A directory a scan visits, from its listing to its record
    struct DirectoryScan
    {
      std::string path;
      SScanSettings settings;
      ADDON::ScraperPtr info;
      ADDON::ContentType content{ADDON::ContentType::NONE};
      bool foundDirectly{false};
      bool isPlugin{false};
      bool isArchive{false};
      //! The directory's items to look up, and its folders to scan
      CFileItemList items;
      //! Whether its listing is unchanged, so that its items aren't looked up
      bool skip{false};
      //! What is recorded for the directory once its items have been added
      std::optional<FolderRecord> newRecord;
      std::vector<std::string> extrasFolders;
      bool extrasAddedAgain{false};
      bool foundSomething{false};
      bool lookupFailed{false};
      //! Whether it holds nothing but folders to scan, and so takes no share of the progress
      bool leftOut{false};
      //! Whether the discs in its folders of extras are queued to be read ahead
      bool extrasReadAhead{false};
      //! The folders it holds to scan, by path, planned before its turn so that their movies are
      //! looked up ahead
      std::vector<std::pair<std::string, std::unique_ptr<DirectoryScan>>> plannedAhead;
      //! The look-up of its items, once set up
      std::unique_ptr<ItemLookups> lookups;
    };

    /*! \brief The first part of DoScan(): whether a directory is scanned, its listing and its items
     \return the directory to scan, or nullptr where it is left alone
     */
    std::unique_ptr<DirectoryScan> PlanDirectory(const std::string& strDirectory);
    //! \brief The second part of DoScan(): the directory's items looked up and added
    void RetrieveDirectory(DirectoryScan& scan);
    //! \brief The look-up of a directory's items a scan makes
    void SetUpLookups(const DirectoryScan& scan, ItemLookups& batch);
    void EndRetrieve(DirectoryScan& scan, bool foundSomething, unsigned int failedLookups);
    //! \brief The last part of DoScan(): extras, discs, the folders below and the record
    std::pair<ScanComplete, ContentFound> FinishDirectory(DirectoryScan& scan);

    //! \brief Whether a directory is scanned with the folders beside it, its movies looked up
    //! alongside theirs
    bool CanScanAlongside(const DirectoryScan& scan) const;
    //! \brief Whether an item of a directory is a folder of extras, added to its movie
    bool AddsAsExtras(const DirectoryScan& scan, const CFileItem& item);
    //! \brief Whether an item of a directory is a folder it scans with those beside it
    bool IsScannedAlongside(const DirectoryScan& scan,
                            const CFileItem& item,
                            bool addAsExtras) const;
    /*! \brief Plan the folders a directory holds to scan, where they are few, as those of a movie's
     discs are, so that their movies are looked up with those of the folders beside it
     \param budget how many more folders may be planned ahead, the folders below them included
     */
    void PlanAhead(DirectoryScan& scan, size_t& budget);
    //! \brief The folders a directory stands for: itself and those it planned ahead
    static size_t CountPlanned(const DirectoryScan& scan);

    /*!
     \brief Count the folders a directory holds to scan, which its listing has found, and leave the
     directory out of the progress where it holds nothing else
     */
    void CountFoldersFound(DirectoryScan& scan);
    //! \brief A folder the scan has finished with
    void FolderFinished();
    //! \brief Show the progress of the whole scan \param share how much of the folder in hand is done
    void ReportProgress(float share = 0.f);

    /*! \brief Scan folders beside each other: their movies are looked up alongside each other, and
     each folder is finished once its own are added. One holding folders to scan scans them as it
     finishes, while the look-ups of those after it go on; the movies of those it planned ahead are
     looked up before its turn.
     */
    void ScanAlongside(std::vector<std::unique_ptr<DirectoryScan>>& folders);

    //! \brief What AddVideo() reads before it writes a video: from the file system, never from
    //! the database, so that it can be read for several videos alongside each other
    struct PreparedVideo
    {
      ADDON::ContentType content{ADDON::ContentType::NONE};
      CVideoInfoScannerArt::UseRemoteArtWithLocalScraper useRemoteArt{
          CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::YES};
      std::string path;
      KODI::ART::Artwork art;
      KODI::ART::SeasonsArtwork seasonArt;
      std::optional<CDateTime> dateAdded;
    };

    /*! \brief The first half of AddVideo(): the item's playlist, art, streams, trailer and date added
     \param regexps the cache of regular expressions of the thread preparing the item
     \param lastProbed the file the thread probed last, and its own streams
     */
    PreparedVideo PrepareVideo(
        CFileItem* pItem,
        const ADDON::ScraperPtr& scraper,
        bool videoFolder,
        bool useLocal,
        const CVideoInfoTag* showInfo,
        bool libraryImport,
        ADDON::ContentType contentOverride,
        KODI::REGEXP::RegExpCache& regexps,
        std::optional<std::pair<std::string, CStreamDetails>>& lastProbed) const;

    //! \brief The second half of AddVideo(): the item written in one transaction, and announced
    long WriteVideo(CFileItem* pItem,
                    const PreparedVideo& video,
                    bool videoFolder,
                    const CVideoInfoTag* showInfo,
                    bool libraryImport);

    struct ShowLookup;
    /*! \param lookup the show as it was looked up already, or nullptr to look it up now
     */
    InfoRet RetrieveInfoForTvShow(CFileItem* pItem,
                                  bool bDirNames,
                                  ADDON::ScraperPtr& scraper,
                                  bool useLocal,
                                  CScraperUrl* pURL,
                                  bool fetchEpisodes,
                                  CGUIDialogProgress* pDlgProgress,
                                  ShowLookup* lookup = nullptr);
    //! \brief How many threads a scan looks videos up on: enough to keep a scraper busy while the
    //! others read and probe
    static size_t LookUpThreads() { return 4; }

    //! \brief What looking a video up found on a thread of its own, for the scan thread to add
    struct VideoLookup
    {
      //! Whether the video was looked up
      bool done{false};
      InfoType nfoResult{InfoType::NONE};
      std::unique_ptr<IVideoInfoTagLoader> loader;
      //! The video as its nfo has it, or as the scraper has it where it was found
      CFileItem item;
      //! Whether the scraper found the video and its details were read
      bool found{false};
      //! What the scraper's search returned, where the video was searched for
      std::optional<int> searchResult;
      //! Whether the search found the video
      bool searchFound{false};
      //! The errors the scraper reported, shown as the video is added
      std::vector<ADDON::CScraperError> errors;
    };

    //! \brief What looking a movie up found, for RetrieveInfoForMovie() to add
    struct MovieLookup : VideoLookup
    {
      std::string edition;
      bool setUpdated{false};
      std::optional<PreparedVideo> prepared;
    };

    /*! \param lookup the movie as it was looked up already, or nullptr to look it up now
     */
    InfoRet RetrieveInfoForMovie(CFileItem* pItem,
                                 bool bDirNames,
                                 ADDON::ScraperPtr& scraper,
                                 bool useLocal,
                                 CScraperUrl* pURL,
                                 CGUIDialogProgress* pDlgProgress,
                                 MovieLookup* lookup = nullptr);

    /*! \brief Whether a movie can be looked up on a thread of its own, alongside others: its nfo,
     the scraper, its art and its streams. A movie a refresh looks up is not.
     */
    bool CanLookUpMovieAlongside(const CFileItem& item) const;

    /*! \brief The part of RetrieveInfoForMovie() that reads the movie's nfo, asks the scraper and
     prepares the movie, which neither writes to the database nor shows a dialog. A disc is prepared
     as it is added, as its playlists are chosen with those the library already has.
     */
    void LookUpMovie(const CFileItem& source,
                     bool bDirNames,
                     const ADDON::ScraperPtr& info2,
                     bool useLocal,
                     bool useFolderNames,
                     MovieLookup& lookup) const;

    //! \brief Where a video's name may give an identifier the scraper can look it up by
    enum class IdentifierIn
    {
      FILE_NAME, //!< as a movie's file is named
      TITLE, //!< as a show's folder, or its nfo, names it
    };

    /*! \brief The part of a look-up that finds the video with the scraper and reads its details: by
     an identifier its name gives, by the url its nfo gives, or by searching for its title, as the
     look-up's nfo result has it. A search is recorded in the look-up.
     \param item the video as its nfo has it
     \param[in,out] found the video, with its details where they were read
     \return whether the video was found and its details read
     */
    bool FindVideoAhead(const CFileItem& source,
                        const CFileItem& item,
                        IdentifierIn identifierIn,
                        bool bDirNames,
                        const ADDON::ScraperPtr& info2,
                        VideoLookup& lookup,
                        CFileItem& found,
                        KODI::REGEXP::RegExpCache& regexps) const;

    /*! \brief What a look-up done ahead that didn't find its video means, as FindVideo() decides on
     what its search returned: the scan stopped, or the video not found
     */
    InfoRet LookupNotFound(const VideoLookup& lookup, CGUIDialogProgress* pDlgProgress);

    //! \brief What looking a show up found on a thread of its own, for RetrieveInfoForTvShow() to add
    struct ShowLookup : VideoLookup
    {
      //! The interpreter the show's scraper calls keep to, from its look-up to its last episode
      std::unique_ptr<CScriptInvocationManager::CAffinity> affinity;
      //! The show's episode guide, where it could be fetched
      std::optional<EPISODELIST> guide;
    };

    /*! \brief The part of RetrieveInfoForTvShow() that reads the show's nfo, asks the scraper for it
     and fetches its episode guide, which neither writes to the database nor shows a dialog. Its
     scraper calls keep to an affinity of its own, which its episodes' calls keep to after.
     */
    void LookUpShow(const CFileItem& source,
                    bool bDirNames,
                    const ADDON::ScraperPtr& info2,
                    bool useLocal,
                    ShowLookup& lookup) const;

    //! \brief Have the discs of a directory's movies about to be looked up read ahead
    void ReadDiscsAhead(const ItemLookups& batch);
    //! \brief Have the discs in a directory's folders of extras read ahead
    void ReadExtrasDiscsAhead(DirectoryScan& scan);
    //! \brief Read what adding a disc's movie reads of the disc, into the disc cache
    void ReadMovieDisc(CFileItem movie) const;
    //! \brief Read what adding a folder of extras reads of the discs in it, into the disc cache
    void ReadExtrasDiscs(const std::string& folder) const;
    //! \brief Have the discs of a show's episodes read ahead, in the order the episodes are added
    void ReadEpisodeDiscsAhead(const std::vector<std::string>& discs);
    /*! \brief The discs of a show new to the scan, in the order its episodes are added, from the
     listings EnumerateSeriesFolder() then lists the show from
     */
    std::vector<std::string> ShowDiscs(const std::string& show);
    //! \brief Read what choosing the playlists of a disc's episodes reads, into the disc cache
    void ReadEpisodeDisc(const std::string& disc) const;

    //! \brief The edition a movie's file or folder name gives
    std::string MovieEdition(const CFileItem& item,
                             bool bDirNames,
                             bool useFolderNames,
                             KODI::REGEXP::RegExpCache& regexps) const;

    //! \brief The items of a directory RetrieveVideoInfo() looks up and adds
    struct ItemLookups
    {
      const CFileItemList* items{nullptr};
      bool bDirNames{false};
      ADDON::ContentType content{ADDON::ContentType::NONE};
      bool useLocal{true};
      CScraperUrl* pURL{nullptr};
      bool fetchEpisodes{true};
      CGUIDialogProgress* pDlgProgress{nullptr};
      //! Whether the folder a movie is in names it, for the items of this directory
      bool useFolderNames{false};
      CVideoDatabase::FolderMovies folderMovies;
      struct Candidate
      {
        ADDON::ScraperPtr info2;
        bool lookUpAlongside{false};
        //! Whether the show is looked up ahead of its turn
        bool lookUpShow{false};
      };
      std::vector<std::optional<Candidate>> candidates;
      std::vector<MovieLookup> lookups;
      std::vector<ShowLookup> shows;
      bool foundSomeInfo{false};
      std::vector<int> seenPaths;
    };

    //! \brief Which of the items are looked up, and with which scraper, before any is
    void PlanLookups(ItemLookups& batch);
    //! \brief Look an item up, on any thread
    void LookUpItem(ItemLookups& batch, size_t i) const;
    //! \brief Add an item once it is looked up \return false to look up no more
    bool HandBackItem(ItemLookups& batch, size_t i);
    //! \brief Once all the items are added
    void EndLookups(ItemLookups& batch);

    /*! \brief The details the scraper has for an item, as GetDetails() reads them, without reporting
     on progress
     */
    bool LookUpDetails(CFileItem& item,
                       const ADDON::CScraper::UniqueIDs& uniqueIDs,
                       CScraperUrl& url,
                       const ADDON::ScraperPtr& scraper,
                       VIDEO::IVideoInfoTagLoader* loader,
                       CGUIDialogProgress* pDialog = nullptr) const;
    InfoRet RetrieveInfoForMusicVideo(CFileItem* pItem,
                                      bool bDirNames,
                                      ADDON::ScraperPtr& scraper,
                                      bool useLocal,
                                      CScraperUrl* pURL,
                                      CGUIDialogProgress* pDlgProgress);
    /*!
     \param showAffinity the interpreter the show's scraper calls keep to, where what the scraper
     fetched for the show is kept
     */
    InfoRet RetrieveInfoForEpisodes(CFileItem* item,
                                    long showID,
                                    EPISODELIST& files,
                                    const ADDON::ScraperPtr& scraper,
                                    bool useLocal,
                                    const CScriptInvocationManager::CAffinity& showAffinity,
                                    CGUIDialogProgress* progress = nullptr,
                                    bool alreadyHasArt = false,
                                    const EPISODELIST* guide = nullptr);

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
     \param nfoFile if set, we override the online data with the locally supplied data. Defaults to nullptr.
     \param pDialog progress dialog to update and check for cancellation during processing. Defaults to nullptr.
     \return true if information is found, false if an error occurred, the lookup was cancelled, or no information was found.
     */
    bool GetDetails(CFileItem* pItem,
                    const ADDON::CScraper::UniqueIDs& uniqueIDs,
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

    /*! \brief Queue a scanned directory to be cleaned, along with the paths holding its media
     A disc rip or archive is not anchored in the folder that was scanned but under a
     bluray:// or zip:// path of its own. Those are queued too, otherwise the clean never
     looks at them and media removed with the disc stays in the library.
     The video extras folders of the directory are queued too, as the scan adds their
     extras without scanning them as directories of their own.
     \param directory the directory that was scanned
     */
    void AddPathToClean(const std::string& directory);

    /*! \brief Process a series folder, filling in episode details and adding them to the database.
     @todo Ideally we would return InfoRet:HAVE_ALREADY if we don't have to update any episodes
     and we should return InfoRet::NOT_FOUND only if no information is found for any of
     the episodes. InfoRet::ADDED then indicates we've added one or more episodes.
     \param files the episode files to process.
     \param scraper scraper to use for finding online info
     \param showInfo information for the show.
     \param pDlgProcess progress dialog to update during processing.  Defaults to nullptr.
     \return InfoRet::ERROR on failure, InfoRet::CANCELLED on cancellation,
     InfoRet::NOT_FOUND if an episode isn't found, or InfoRet::ADDED if all episodes are added.
     */
    InfoRet OnProcessSeriesFolder(EPISODELIST& files,
                                  const ADDON::ScraperPtr& scraper,
                                  bool useLocal,
                                  const CVideoInfoTag& showInfo,
                                  const CScriptInvocationManager::CAffinity& showAffinity,
                                  CGUIDialogProgress* pDlgProgress = nullptr,
                                  const EPISODELIST* fetchedGuide = nullptr);

    enum class EpisodeResult
    {
      NO_MEDIA, //!< .nomedia file is present
      NO_FILES, //!< No episode candidate files found
      NO_EPISODES, //!< Episode candidate files found, but none could be parsed
      NOT_CHANGED, //!< No new episodes found (directory hash unchanged)
      FOUND_EPISODES //!< Episodes found
    };

    EpisodeResult EnumerateSeriesFolder(CFileItem* item, EPISODELIST& episodeList);

    /*! \brief The episodes of the files listed for a show, of which a disc structure gives its index
     \return whether any of the files could hold an episode
     */
    bool EnumerateShowFiles(CFileItemList& items, EPISODELIST& episodeList);

    //! \brief As CVideoDatabase::GetScraperForPath(), from the rows of the path table the scan read
    ADDON::ScraperPtr ScraperForPath(const std::string& path,
                                     SScanSettings& settings,
                                     bool& foundDirectly);
    ADDON::ScraperPtr ScraperForPath(const std::string& path)
    {
      SScanSettings settings;
      bool foundDirectly{false};
      return ScraperForPath(path, settings, foundDirectly);
    }

    /*! \brief A digest of everything that changes what a folder is scanned for, which its record is
     stored with, so that a folder scanned otherwise before is scanned again
     \param folder the folder, with a slash at its end
     \return the digest, empty where the folder has no content
     */
    std::string FolderSettings(const std::string& folder);

    //! \brief The listing of a folder, taken unless the scan took it already
    std::shared_ptr<const CDirectorySnapshot> Snapshot(const std::string& folder);

    //! \brief The modified time of a folder, as its parent's listing gave it or read now
    std::optional<int64_t> FolderTime(const std::string& folder);

    /*! \brief The folders of a listing that a scan looks into: neither hidden, nor part of a disc
     structure, nor excluded. Their times are kept for FolderTime().
     */
    std::vector<std::string> ScannedSubfolders(const CDirectorySnapshot& snapshot,
                                               const std::vector<std::string>& excludes);
    bool HasScannedSubfolders(const CDirectorySnapshot& snapshot,
                              const std::vector<std::string>& excludes);

    //! \brief Whether a folder's record shows it unchanged, without listing it
    bool FolderUnchangedUnlisted(const std::string& folder);

    /*! \brief Whether a folder holds .nomedia, from its record where it is unchanged
     \param list whether to list the folder, which a scan is about to, rather than look into it
     */
    bool FolderHasNoMedia(const std::string& folder, bool list = false);

    /*! \brief Whether anything in a folder, or in a folder below it, has changed since their records
     were stored. Only the folders with folders of their own to look into are listed.
     */
    bool FolderChanged(const std::string& folder, const std::vector<std::string>& excludes);

    //! \brief The records of a folder and of the folders below it, from listings of them
    std::vector<std::pair<std::string, FolderRecord>> ListFolderRecords(
        const std::string& folder, const std::vector<std::string>& excludes);
    void StoreFolderRecords(const std::string& folder, const std::vector<std::string>& excludes);

    /*! \brief Add the extras folders beside a movie's disc that have changed since they were added
     \param discFolder the listing of the folder holding the disc structure
     \return whether an extras folder was added again, so may have lost an extra
     */
    bool AddExtrasBesideDisc(const CDirectorySnapshot& discFolder);

    /*! \brief The files of a show that has changed, from a listing of each of its folders, and the
     records of those folders
     \param recorded whether the folder is one a scan records
     */
    void CollectShowFiles(const std::string& folder,
                          CFileItemList& files,
                          std::vector<std::pair<std::string, FolderRecord>>& records,
                          bool recorded = true);

    //! \brief Record the folders of a show once its episodes have been added
    void StoreShowRecords(const std::string& show);

    //! \brief Forget the listings of a folder and of those below it, once the scan has left them
    void ForgetListings(const std::string& folder);
    bool ProcessItemByVideoInfoTag(const CFileItem *item, EPISODELIST &episodeList);

    std::pair<VersionConversionResult, int> ProcessVideoVersion(VideoDbContentType itemType,
                                                                int dbId,
                                                                int targetDbId = -1,
                                                                bool canBecomeDefault = true);
    static void RemovePartNumberFromTitle(int dbId,
                                          VideoDbContentType itemType,
                                          CVideoDatabase& db);

    /*!
     * \brief Add bluray playlists found for the same disc as movie and/or versions.
     * \param[in] blurayItems all candidate playlists found for the disc
     * \param[in] scraper scraper used for the lookup
     * \param[in] bDirNames whether directory names are used for identification
     * \param[in] useLocal whether to use local information for artwork etc.
     * \return InfoRet::INFO_ERROR on failure, InfoRet::ADDED if movie added, InfoRet::HAVE_ALREADY if versions added
     */
    InfoRet AddBlurayPlaylistVersions(const CFileItemList& blurayItems,
                                      const ADDON::ScraperPtr& scraper,
                                      bool bDirNames,
                                      bool useLocal);

    std::pair<InfoType, std::unique_ptr<IVideoInfoTagLoader>> ReadInfoTag(
        CFileItem& item, const ADDON::ScraperPtr& scraper, bool lookInFolder, bool resetTag) const;

    //! Sticky - never reset, so a scanner instance is good for one scan only
    std::atomic<bool> m_bStop{false};
    bool m_scanAll;
    bool m_scanSubtree{false};

    //! Lookups so far that failed, rather than finding nothing. A directory in which one failed
    //! isn't hashed, so that the next scan tries it again.
    unsigned int m_failedLookups{0};

    //! What the scan did, logged once it has finished
    struct ScanSummary
    {
      enum class Type
      {
        MOVIE,
        VERSION, //!< of a movie
        TV_SHOW,
        EPISODE,
        MUSIC_VIDEO
      };

      struct Item
      {
        Type type;
        std::string path;
        std::string title; //!< of an item added
      };

      std::vector<Item> added;
      std::vector<Item> existing; //!< items in rescanned directories already in the library
      unsigned int unchangedDirectories{0};
      std::vector<Item> notFound;
      std::vector<Item> failed;
    };
    ScanSummary m_summary;

    SimilarVideoScanAction m_similarVideoAction{SimilarVideoScanAction::NONE};
    bool m_ignoreVideoExtras{false};

    //! Whether RetrieveVideoInfo() leaves a movie's extras to AddMovieExtras()
    bool m_deferMovieExtras{false};

    //! The ids RetrieveVideoInfo() looks movies and tv shows up by, where it would otherwise search
    ADDON::CScraper::UniqueIDs m_uniqueIDs;

    //! Whether the folder a movie is in names it (the scraper's "movies are in separate folders")
    bool m_useFolderNames{false};

    CVideoDatabase m_database;
    std::set<int> m_pathsToClean;
    std::shared_ptr<CAdvancedSettings> m_advancedSettings;
    CVideoDatabase::ScraperCache m_scraperCache;

    //! The artwork side of the scan
    CVideoInfoScannerArt m_art;
    CVideoInfoScannerExtras m_extras{m_database, m_art};

    //! \brief The scraper's downloads in flight, no more at once than a scan has threads looking
    //! videos up, which Stop() gives up
    class CDownloads
    {
    public:
      //! \param calls how many downloads may go ahead at once, the others waiting for a place
      explicit CDownloads(size_t calls) : m_free(calls > 0 ? calls : 1) {}

      //! \brief Gives up the downloads in flight, and lets those waiting go ahead, given up at once
      void CancelAll();

      //! \brief A download the scan waits on, for as long as the registration lives, which waits
      //! for a place
      class CRegistration
      {
      public:
        CRegistration(CDownloads& downloads, CVideoInfoDownloader& downloader);
        ~CRegistration();
        CRegistration(const CRegistration&) = delete;
        CRegistration& operator=(const CRegistration&) = delete;

      private:
        CDownloads& m_downloads;
        CVideoInfoDownloader& m_downloader;
        //! Whether it took a place, which one a stop let through doesn't
        bool m_placed{false};
      };

    private:
      std::mutex m_lock;
      std::condition_variable m_freed;
      std::set<CVideoInfoDownloader*> m_downloaders;
      size_t m_free;
      bool m_cancelled{false};
    };
    mutable CDownloads m_downloads{LookUpThreads()};

    //! The progress of the whole scan, by folder
    CScanProgress m_progress;
    //! The shows of a source counted as found, finished once their episodes are added
    std::set<std::string, std::less<>> m_progressShows;

    //! The rows of the path table below the folders scanned, read once
    std::unique_ptr<CScanPathTable> m_pathTable{std::make_unique<CScanPathTable>(m_database)};
    //! Listings taken of folders the scan has yet to leave
    std::unordered_map<std::string, std::shared_ptr<const CDirectorySnapshot>> m_snapshots;
    //! The order the listings were taken in, the oldest let go of first
    std::deque<std::string> m_snapshotOrder;
    //! The modified time of each folder as its parent's listing gave it
    std::unordered_map<std::string, std::optional<int64_t>> m_listedTimes;
    //! The file probed last, and its own streams, which its other episodes have too
    std::optional<std::pair<std::string, CStreamDetails>> m_lastProbed;
    //! The movies of the folder whose items are being added
    const CVideoDatabase::FolderMovies* m_folderMovies{nullptr};
    //! The settings digest of each folder looked at
    std::unordered_map<std::string, std::string> m_folderSettings;
    //! The records of the folders of each show, stored once its episodes are added
    std::unordered_map<std::string, std::vector<std::pair<std::string, FolderRecord>>>
        m_showRecords;

  private:
    /*!
     * \brief Count an item that wasn't added in the summary, and log it as an event
     * \param[in] type What the item is
     * \param[in] path Path of the item
     * \param[in] failed Whether its lookup failed, rather than found nothing
     */
    void NotAdded(ScanSummary::Type type, const std::string& path, bool failed);
    void ReportSummary() const;

    /*!
     * \brief Remove paths that share missing ancestors with \p directory from the list of paths
     *        to scan
     * \param[in] directory The non-existent directory
     */
    void SkipRelatedDirectories(std::string_view directory);

    /*!
     * \brief Removes the directory and sub directories of \p directory from the paths to be
     *        scanned. Paths must end with a directory separator.
     * \param[in] directories List of paths
     * \param[in] directory Path of the directory to remove
     * \param[in] f function to execute before the removal of a path
     * \return number of elements removed
     */
    static size_t RemoveSubDirectories(std::set<std::string, std::less<>>& directories,
                                       std::string_view directory,
                                       std::function<void(const std::string&)> f);

    //! \brief Read the editions known to the library, once
    void LoadVideoVersionTypes();

    /*!
     * \brief Look for the name of an edition known to the library (ex. "Director's Cut") within the
     *        name of the folder holding a disc, once the editions have been read. The longest match
     *        wins, and a match at the very start of the folder name is ignored as that is a movie
     *        whose title is an edition name (ex. "The Final Cut (2004)") rather than a version of
     *        another movie.
     * \param[in] folderName Name of the folder holding the disc
     * \return The edition's name as held in the library, empty if none was recognised
     */
    std::string FindEditionInFolderName(const std::string& folderName) const;

    mutable KODI::REGEXP::RegExpCache m_regexpCache;

    //! Editions known to the library, cached for the duration of a scan
    std::vector<std::string> m_videoVersionTypes;
    bool m_videoVersionTypesCached{false};

    //! Reads discs ahead of the scan, two at a time. Last, so that its threads are joined before
    //! what they read with goes. Mutable, as a look-up drops a disc the scan turns out not to need.
    mutable CDiscReadAhead m_discReadAhead{4, 2};
  };
  } // namespace KODI::VIDEO
