/*
 *  Copyright (C) 2016-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Bookmark.h"
#include "DiscPlaylist.h"
#include "EpisodeFileMap.h"
#include "VideoDbContentType.h"
#include "VideoInfoTag.h"
#include "addons/Scraper.h"
#include "dbwrappers/Database.h"
#include "media/MediaType.h"
#include "utils/Artwork.h"
#include "utils/SortUtils.h"
#include "utils/UrlOptions.h"
#include "video/geometry/ContentGeometryRecord.h"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

class CFileItem;
class CFileItemList;
class CStreamDetails;
class CVideoDbUrl;
class CVideoSettings;
class CGUIDialogProgress;
class CGUIDialogProgressBarHandle;
class TiXmlNode;

struct VideoAssetInfo;

enum class VideoAssetTypeOwner;
enum class VideoAssetType;

namespace dbiplus
{
  class field_value;
  using sql_record = std::vector<field_value>;
}

namespace KODI::VIDEO
{
  class IVideoInfoScannerObserver;
  struct SScanSettings;
}

/*!
 * \brief The details an edit changed, as passed to CVideoDatabase::UpdateDetailsForMovie.
 */
namespace KODI::VIDEO::UPDATED_DETAIL
{
inline constexpr char ART[] = "art.altered";
inline constexpr char ARTIST[] = "artist";
inline constexpr char COUNTRY[] = "country";
inline constexpr char DATE_ADDED[] = "dateadded";
inline constexpr char DIRECTOR[] = "director";
inline constexpr char GENRE[] = "genre";
inline constexpr char RATINGS[] = "ratings";
inline constexpr char SET[] = "set";
inline constexpr char SHOW_LINK[] = "showlink";
inline constexpr char STUDIO[] = "studio";
inline constexpr char TAG[] = "tag";
inline constexpr char UNIQUE_ID[] = "uniqueid";
inline constexpr char WRITER[] = "writer";
} // namespace KODI::VIDEO::UPDATED_DETAIL

enum VideoDbDetails
{
  VideoDbDetailsNone     = 0x00,
  VideoDbDetailsRating   = 0x01,
  VideoDbDetailsTag      = 0x02,
  VideoDbDetailsShowLink = 0x04,
  VideoDbDetailsStream   = 0x08,
  VideoDbDetailsCast     = 0x10,
  VideoDbDetailsBookmark = 0x20,
  VideoDbDetailsUniqueID = 0x40,
  VideoDbDetailsAll      = 0xFF
} ;

enum class ArtFallbackOptions
{
  NONE,
  PARENT
};

enum class DeleteMovieCascadeAction
{
  DEFAULT_VERSION,
  ALL_ASSETS,
  ALL_ASSETS_NOT_STREAMDETAILS
};

enum class DeleteMovieHashAction
{
  HASH_DELETE,
  HASH_PRESERVE
};

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

static constexpr const char* MULTIPLE_EPISODES{"multiple_episodes"};

//! \brief One file the content geometry sweep may have to measure, and what it already holds.
struct ContentGeometryCandidate
{
  int idFile{-1};
  std::string path;
  std::optional<KODI::VIDEO::GEOMETRY::ContentGeometryRecord> stored;
};

class CVideoDatabase : public CDatabase
{
  struct FileInformation
  {
    // user-defined ctor required for XCode 15.2 and emplace_back
    FileInformation(std::string&& newPath, int newFileId, int newVvId, std::string&& newHash);

    std::string path;
    int fileId{0};
    int vvId{0};
    std::string hash;
  };

public:

  class CActor    // used for actor retrieval for non-master users
  {
  public:
    std::string name;
    std::string thumb;
    int playcount;
    int appearances;
  };

  class CSeason   // used for season retrieval for non-master users
  {
  public:
    std::string path;
    std::vector<std::string> genre;
    int numEpisodes;
    int numWatched;
    int id;
  };

  class CSetInfo
  {
  public:
    std::string name;
    std::vector<CVideoInfoTag> movies;
    DatabaseResults results;
  };

  CVideoDatabase();
  ~CVideoDatabase() override;

  bool Open() override;
  bool CommitTransaction() override;

  //! \brief Whether play count and resume point writes are announced; off for values restored
  //! from an NFO file or an earlier record of the item, which change nothing a listener knew.
  void SetAnnounceFileChanges(bool announce) { m_announceFileChanges = announce; }

  int AddNewEpisode(int idShow, CVideoInfoTag& details);

  // editing functions
  /*! \brief Set the playcount of an item, update last played time
   Sets the playcount and last played date to a given value
   \param item CFileItem to set the playcount for
   \param count The playcount to set.
   \param date The date the file was last viewed (does not denote the video was watched to completion).
   If empty we use current datetime (if count > 0) or never viewed (if count = 0).
   A count other than the database held, or the item showed, is announced as
   Files.OnPlayCountChanged.
   \return on success, the new last played time set, invalid datetime otherwise.
   \sa GetPlayCount, IncrementPlayCount, UpdateLastPlayed
   */
  CDateTime SetPlayCount(const CFileItem& item, int count, const CDateTime& date = CDateTime());

  /*! \brief Increment the playcount of an item
   Increments the playcount and updates the last played date
   \param item CFileItem to increment the playcount for
   \return on success, the new last played time set, invalid datetime otherwise.
   \sa GetPlayCount, SetPlayCount, GetPlayCounts
   */
  CDateTime IncrementPlayCount(const CFileItem& item);

  /*! \brief Get the playcount of an item
   \param item CFileItem to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(const CFileItem &item);

  /*! \brief Get the playcount of a filename and path
   \param strFilenameAndPath filename and path to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(const std::string& strFilenameAndPath);

  /*!
   * \brief Whether a library item counts as watched: a TV show while any of its episodes is, a
   * movie, episode or music video while it has a play count.
   */
  bool IsWatched(KODI::MEDIA::TYPE type, int id) const;

  /*!
   * \brief Whether an edit may clear the last played date of a library item or a file: one left
   * watched keeps its date, and a date alone never changes the play count.
   * \param playCount the play count the same edit sets, nullopt if it keeps the one stored
   */
  bool CanClearLastPlayed(KODI::MEDIA::TYPE type, int id, std::optional<int> playCount) const;
  bool CanClearLastPlayed(const std::string& file, std::optional<int> playCount);

  /*! \brief Get the last played time of a filename and path
   \param strFilenameAndPath filename and path to get the last played time for
   \return the last played time of the item, or an invalid CDateTime on error
   \sa UpdateLastPlayed
   */
  CDateTime GetLastPlayed(const std::string& strFilenameAndPath);

  /*! \brief Update the last played time of an item
   Updates the last played date
   \param item CFileItem to update the last played time for
   \return on success, the last played time set, invalid datetime otherwise.
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  CDateTime UpdateLastPlayed(const CFileItem& item);

  /*! \brief Get the playcount and resume point of a list of items
   Note that if the resume point is already set on an item, it won't be overridden.
   \param path the path to fetch videos from
   \param items CFileItemList to fetch the playcounts for
   \sa GetPlayCount, SetPlayCount, IncrementPlayCount
   */
  bool GetPlayCounts(const std::string &path, CFileItemList &items);

  void UpdateMovieTitle(int idMovie,
                        const std::string& strNewMovieTitle,
                        VideoDbContentType iType = VideoDbContentType::MOVIES);

  /*! \brief Set the sort title of a movie, tvshow or movie set.
   \param[in] idDb the dbId of the item
   \param[in] strNewSortTitle the new sort title
   \param[in] iType the content type of the item
   \return true on success.
   */
  bool UpdateVideoSortTitle(int idDb,
                            const std::string& strNewSortTitle,
                            VideoDbContentType iType = VideoDbContentType::MOVIES);

  bool HasMovieInfo(const std::string& strFilenameAndPath);
  bool HasTvShowInfo(const std::string& strFilenameAndPath);
  bool HasEpisodeInfo(const std::string& strFilenameAndPath);
  bool HasMusicVideoInfo(const std::string& strFilenameAndPath);

  //! \brief Whether the library holds an item, as a movie, an episode or a music video by its file,
  //! or as a TV show by its folder
  bool HasVideoInfo(const CFileItem& item);

  void GetFilePathById(int idMovie, std::string& filePath, VideoDbContentType iType);
  std::string GetGenreById(int id) const;
  std::string GetCountryById(int id) const;
  std::string GetSetById(int id) const;
  std::string GetOriginalSetById(int id) const;
  std::string GetTagById(int id) const;
  std::string GetPersonById(int id) const;
  std::string GetStudioById(int id) const;
  std::string GetTvShowTitleById(int id) const;
  std::string GetMusicVideoAlbumById(int id) const;
  int GetTvShowForEpisode(int idEpisode) const;
  int GetSeasonForEpisode(int idEpisode) const;

  /*!
   * \brief Fill details with what the library holds for a file. An extra's file is filled as the
   * extras list of its movie shows it.
   */
  bool LoadVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details);
  bool GetMovieInfo(const std::string& strFilenameAndPath,
                    CVideoInfoTag& details,
                    int idMovie = -1,
                    int idVersion = -1,
                    int idFile = -1,
                    int getDetails = VideoDbDetailsAll);
  bool GetTvShowInfo(const std::string& strPath,
                     CVideoInfoTag& details,
                     int idTvShow = -1,
                     CFileItem* item = nullptr,
                     int getDetails = VideoDbDetailsAll);
  bool GetSeasonInfo(const std::string& path, int season, CVideoInfoTag& details, CFileItem* item);
  bool GetSeasonInfo(int idSeason,
                     CVideoInfoTag& details,
                     CFileItem* item = nullptr,
                     bool allDetails = true);
  bool GetEpisodeBasicInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode  = -1);
  bool GetEpisodeInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode = -1, int getDetails = VideoDbDetailsAll);
  bool GetMusicVideoInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMVideo = -1, int getDetails = VideoDbDetailsAll);
  bool GetSetInfo(int idSet, CVideoInfoTag& details, CFileItem* item = nullptr);
  bool GetFileInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idFile = -1);

  // Asked by path, these answer NotFound for a path they cannot resolve to an id, even when it
  // is that lookup which failed.

  //! With idMovie given, the path only picks the version, when neither idVersion nor idFile does.
  GetResult TryGetMovieInfo(const std::string& strFilenameAndPath,
                            CVideoInfoTag& details,
                            int idMovie = -1,
                            int idVersion = -1,
                            int idFile = -1,
                            int getDetails = VideoDbDetailsAll);

  //! The path is ignored when idTvShow is given.
  GetResult TryGetTvShowInfo(const std::string& strPath,
                             CVideoInfoTag& details,
                             int idTvShow = -1,
                             CFileItem* item = nullptr,
                             int getDetails = VideoDbDetailsAll);

  //! allDetails fills from the season view rather than the season row.
  GetResult TryGetSeasonInfo(int idSeason,
                             CVideoInfoTag& details,
                             CFileItem* item = nullptr,
                             bool allDetails = true);

  //! The path is ignored when idEpisode is given.
  GetResult TryGetEpisodeInfo(const std::string& strFilenameAndPath,
                              CVideoInfoTag& details,
                              int idEpisode = -1,
                              int getDetails = VideoDbDetailsAll);

  //! The path is ignored when idMVideo is given.
  GetResult TryGetMusicVideoInfo(const std::string& strFilenameAndPath,
                                 CVideoInfoTag& details,
                                 int idMVideo = -1,
                                 int getDetails = VideoDbDetailsAll);

  GetResult TryGetSetInfo(int idSet, CVideoInfoTag& details, CFileItem* item = nullptr);

  //! The path is ignored when idFile is given. Adds to details rather than replacing them.
  GetResult TryGetFileInfo(const std::string& strFilenameAndPath,
                           CVideoInfoTag& details,
                           int idFile = -1);

  int GetPathId(const std::string& strPath);

  /*! \brief Get the id of this fileitem
   Works for both videodb:// items and normal fileitems
   \param item CFileItem to grab the fileid of
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const CFileItem& item);
  /*! \brief Get the id of a path, also accepting the zip:// or archive:// equivalent of an
   *         archive path (AddPath() stores these interchangeably).
   */
  int GetArchiveOrAliasPathId(const std::string& strPath);
  int GetTvShowId(const std::string& strPath);
  // input value is episode/season number hint - for multiparters
  int GetEpisodeId(const std::string& strFilenameAndPath, int episode = -1, int season = -1);
  int GetSeasonId(int idShow, int season) const;

  void GetEpisodesByBlurayPath(const std::string& path, std::vector<CVideoInfoTag>& episodes);
  void GetEpisodesByBasePath(const std::string& path,
                             std::vector<CVideoInfoTag>& episodes,
                             int idShow = -1);
  void GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes);
  void GetEpisodesByFileId(int idFile, std::vector<CVideoInfoTag>& episodes);
  bool GetEpisodeMap(int idShow, EpisodeFileMap& fileMap, int idFile = -1) const;
  bool GetEpisodeMap(int idShow,
                     EpisodeFileMap& fileMap,
                     dbiplus::Dataset& pDS,
                     int idFile = -1 /* = -1 */) const;

  int SetDetailsForMovie(CVideoInfoTag& details,
                         const KODI::ART::Artwork& artwork,
                         int idMovie = -1);
  int SetDetailsForMovieSet(const CVideoInfoTag& details,
                            const KODI::ART::Artwork& artwork,
                            int idSet = -1);

  /*! \brief add a tvshow to the library, setting metadata detail
   First checks for whether this TV Show is already in the database (based on idTvShow, or via GetMatchingTvShow)
   and if present adds the paths to the show.  If not present, we add a new show and set the show metadata.
   \param paths a vector<string,string> list of the path(s) and parent path(s) for the show.
   \param details a CVideoInfoTag filled with the metadata for the show.
   \param artwork the artwork map for the show.
   \param seasonArt the artwork map for seasons.
   \param idTvShow the database id of the tvshow if known (defaults to -1)
   \return the id of the tvshow.
   */
  int SetDetailsForTvShow(const std::vector<std::string>& paths,
                          CVideoInfoTag& details,
                          const KODI::ART::Artwork& artwork,
                          const KODI::ART::SeasonsArtwork& seasonArt,
                          int idTvShow = -1);
  bool UpdateDetailsForTvShow(int idTvShow,
                              CVideoInfoTag& details,
                              const KODI::ART::Artwork& artwork,
                              const KODI::ART::SeasonsArtwork& seasonArt);
  int SetDetailsForSeason(const CVideoInfoTag& details,
                          const KODI::ART::Artwork& artwork,
                          int idShow,
                          int idSeason = -1);
  int SetDetailsForEpisode(CVideoInfoTag& details,
                           const KODI::ART::Artwork& artwork,
                           int idShow,
                           int idEpisode = -1);

  /*!
   * @brief The FileInfo structure represents a DB record of the files table.
   */
  struct FileRecord
  {
    int m_idFile{-1};
    int m_playCount{-1};
    CDateTime m_lastPlayed{};
    CDateTime m_dateAdded{};
  };

  int SetFileForMedia(const std::string& fileAndPath,
                      VideoDbContentType type,
                      int mediaId,
                      const FileRecord& oldFile);

  int SetDetailsForMusicVideo(CVideoInfoTag& details,
                              const KODI::ART::Artwork& artwork,
                              int idMVideo = -1);
  bool SetStreamDetailsForFile(const CStreamDetails& details,
                               const std::string& strFileNameAndPath);

  /*!
   * \brief Clear any existing stream details and add the new provided details to a file.
   * \param[in] details New stream details
   * \param[in] idFile Identifier of the file
   * \return operation success. true for success, false for failure
   */
  bool SetStreamDetailsForFileId(const CStreamDetails& details, int idFile);

  /*!
   * \brief Get all playlists from a single bluray:// path in the database
   * \param[in] path The bluray:// path
   * \return vector array of playlist numbers and idFiles
   */
  std::vector<KODI::VIDEO::DiscPlaylist> GetPlaylistsByPath(const std::string& path);

  /*!
   * \brief Announce that a library item has changed, so that widgets and other listeners reload it
   * \param[in] content The item's media type
   */
  static void AnnounceUpdate(KODI::MEDIA::TYPE content, int id);

  //! \brief Set a movie's trailer, announcing the change
  void SetTrailerForMovie(int idMovie, const std::string& trailer);

  bool SetSingleValue(VideoDbContentType type, int dbId, int dbField, const std::string& strValue);
  bool SetSingleValue(VideoDbContentType type,
                      int dbId,
                      Field dbField,
                      const std::string& strValue);
  bool SetSingleValue(const std::string &table, const std::string &fieldName, const std::string &strValue,
                      const std::string &conditionName = "", int conditionValue = -1);

  int UpdateDetailsForMovie(int idMovie,
                            CVideoInfoTag& details,
                            const KODI::ART::Artwork& artwork,
                            const std::set<std::string, std::less<>>& updatedDetails);

  //! \brief A change a caller makes to how far something has been watched; what is empty is kept
  struct PlayStateEdit
  {
    std::optional<int> playCount;
    std::optional<CDateTime> lastPlayed; //!< an invalid date clears it
    struct Resume
    {
      double position{0.0}; //!< 0 or less clears it
      double total{0.0}; //!< as SetResumePoint takes it
    };
    std::optional<Resume> resume;
  };

  enum class EditResult
  {
    DONE,
    REFUSED, //!< it would clear the last played date of what is left watched
    NOT_FOUND, //!< an episode or season without its show, or a file that cannot be read back
    FAILED,
  };

  /*!
   * \brief Store a caller's edit of a library item in one transaction: \p details as the edit
   * leaves them, with \p artwork and without \p removedArtwork, and \p playState on its file. A TV
   * show's play state is given to each of its episodes, which are announced; the item itself is
   * left for the caller to announce with what it changed.
   * \param updatedDetails the details whose links are rewritten, as UpdateDetailsForMovie takes them
   */
  EditResult EditItem(KODI::MEDIA::TYPE type,
                      int id,
                      CVideoInfoTag& details,
                      const KODI::ART::Artwork& artwork,
                      const std::set<std::string, std::less<>>& removedArtwork,
                      const std::set<std::string, std::less<>>& updatedDetails,
                      const PlayStateEdit& playState);

  enum class AddResult
  {
    ADDED,
    IN_LIBRARY, //!< the library already holds an item of the type at the path
    NO_SUCH_SHOW,
    FAILED,
  };

  /*!
   * \brief Add a movie, TV show, episode or music video at a path in one transaction, with
   * \p details, \p artwork and \p playState. One without a title is named after its path. The item
   * is left for the caller to announce.
   * \param idShow the show an episode is added to
   * \param id [out] the new item's id
   */
  AddResult AddItem(KODI::MEDIA::TYPE type,
                    const std::string& path,
                    CVideoInfoTag& details,
                    const KODI::ART::Artwork& artwork,
                    int idShow,
                    const PlayStateEdit& playState,
                    int& id);

  /*!
   * \brief Store a caller's change to a file's play state in one transaction, adding the file if
   * the library has never seen it.
   * \param details [out] the file's state afterwards
   */
  EditResult EditFilePlayState(const std::string& file,
                               const PlayStateEdit& playState,
                               CVideoInfoTag& details);

  /*!
   * \brief Set what a source holds and how it is scanned, keeping whether it uses all external
   * audio, which no caller sets. What the library holds below it goes first when \p removeContent.
   */
  void SetSourceContent(const std::string& path,
                        const ADDON::ScraperPtr& scraper,
                        const KODI::VIDEO::SScanSettings& settings,
                        bool removeContent);

  /*!
   * \brief Describe a path as the library holds it: the movie, episode or music video it is the
   * file of, or for a folder the TV show it is one of the folders of. Otherwise the play state the
   * files table holds for it is added to what \p item already says.
   * \return whether the library holds anything for the path
   */
  bool DescribePath(const std::string& path, CFileItem& item);

  /*!
   * \brief Remove a movie from the library.
   * \param[in] idMovie The id of the movie
   * \param[in] action Versions of the movie to be deleted
   * \param[in] hashAction Preserve or invalidate the hash of the movie path
   * \return operation success. true for success, false for failure
   */
  bool DeleteMovie(int idMovie,
                   DeleteMovieCascadeAction action = DeleteMovieCascadeAction::ALL_ASSETS,
                   DeleteMovieHashAction hashAction = DeleteMovieHashAction::HASH_DELETE);
  void DeleteTvShow(int idTvShow, bool bKeepId = false);
  void DeleteTvShow(const std::string& strPath);
  void DeleteSeason(int idSeason, bool bKeepId = false);

  /*!
   * \brief Remove an episode from the library.
   * \param[in] idEpisode The id of the episode
   * \param[in] bKeepId Keep the episode's entry, so that its details are updated in place
   * \param[in] hashAction Preserve or invalidate the hash of the episode's path, and its show's
   */
  void DeleteEpisode(int idEpisode,
                     bool bKeepId = false,
                     DeleteMovieHashAction hashAction = DeleteMovieHashAction::HASH_DELETE);
  void DeleteMusicVideo(int idMusicVideo, bool bKeepId = false);
  void DeleteDetailsForTvShow(int idTvShow);
  void DeleteStreamDetails(int idFile);
  void RemoveContentForPath(const std::string& strPath, CGUIDialogProgress* progress = nullptr);
  void UpdateFanart(const CFileItem& item, VideoDbContentType type);
  void DeleteSet(int idSet);

  /*!
   * \brief Remove a movie, movie set, TV show, episode or music video from the library.
   * \return false for another kind of item, or if a movie could not be removed
   */
  bool DeleteItem(VideoDbContentType type, int id);
  void DeleteTag(int idTag, VideoDbContentType mediaType);
  bool DeleteFile(int idFile);

  /*! \brief Get video settings for the specified file id
   \param idFile file id to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(int idFile, CVideoSettings &settings);

  /*! \brief Get video settings for the specified file item
   \param item item to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(const CFileItem &item, CVideoSettings &settings);

  /*! \brief Get video settings for the specified file path
   \param filePath filepath to get the settings for
   \return true if video settings found, false otherwise
   \sa SetVideoSettings
   */
  bool GetVideoSettings(const std::string &filePath, CVideoSettings &settings);

  /*! \brief Set video settings for the specified file path
   \param fileItem to set the settings for
   \sa GetVideoSettings
   */
  void SetVideoSettings(const CFileItem &item, const CVideoSettings &settings);

  /*! \brief Set video settings for the specified file path
   \param fileId to set the settings for
   \sa GetVideoSettings
   */
  void SetVideoSettings(int idFile, const CVideoSettings &settings);

  /**
   * Erases video settings for file item
   * @param fileitem
   */
  void EraseVideoSettings(const CFileItem &item);

  /**
   * Erases all video settings
   */
  void EraseAllVideoSettings();

  /**
   * Erases video settings for files starting with path
   * @param path pattern
   */
  void EraseAllVideoSettings(const std::string& path);

  /**
   * Erases all entries for files starting with path, including the files and path entries
   * @param path pattern
   */
  void EraseAllForPath(const std::string& path);

  /*! \brief Erases all entries for the given file, including path entry if no longer used.
   \param fileNameAndPath The name and path of the file to erase db entries for.
   \return True on success, false otherwise.
   */
  bool EraseAllForFile(const std::string& fileNameAndPath);

  bool GetStackTimes(const std::string& filePath, std::vector<std::chrono::milliseconds>& times);
  void SetStackTimes(const std::string& filePath,
                     const std::vector<std::chrono::milliseconds>& times);

  void GetBookMarksForFile(const std::string& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD, bool bAppend=false, long partNumber=0);

  //! \brief Writing a resume point announces it as Files.OnResumePointChanged.
  bool AddBookMarkToFile(const std::string& strFilenameAndPath,
                         const CBookmark& bookmark,
                         CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const std::string& strFilenameAndPath, CBookmark &bookmark);

  /*!
   * \brief Moves a file's resume point, or clears it at a position of 0 or less.
   * \param total the file's length; 0 or less keeps the stored length, else takes the length of its
   * video stream
   */
  bool SetResumePoint(const std::string& file, double position, double total);

  //! \brief Clears the resume point of an item's file, as ClearBookMarksOfFile does.
  void DeleteResumeBookMark(const CFileItem& item);
  void ClearBookMarkOfFile(const std::string& strFilenameAndPath,
                           const CBookmark& bookmark,
                           CBookmark::EType type = CBookmark::STANDARD);

  /*!
   * \brief Clears the bookmarks of a type from a file. Clearing a resume point the database held
   * announces it as Files.OnResumePointChanged.
   * \param shownResumePoint whether the item the file was listed as showed a resume point held
   * outside the database, as a plugin item or a recording can, so that clearing it is announced
   */
  bool ClearBookMarksOfFile(const std::string& strFilenameAndPath,
                            CBookmark::EType type = CBookmark::STANDARD,
                            bool shownResumePoint = false);
  bool ClearBookMarksOfFile(int idFile, CBookmark::EType type = CBookmark::STANDARD);

  //! \brief Clears every bookmark of a file, its resume point included, as ClearBookMarksOfFile
  //! does for each type
  bool ClearAllBookMarksOfFile(const std::string& strFilenameAndPath);
  bool GetBookMarkForEpisode(int dbId, CBookmark& bookmark) const;
  bool GetBookMarkForEpisode(const CVideoInfoTag& tag, CBookmark& bookmark) const;
  void AddBookMarkForEpisode(const CVideoInfoTag& tag, const CBookmark& bookmark);
  void DeleteBookMarkForEpisode(const CVideoInfoTag& tag);
  void DeleteBookMarkForEpisode(int idEpisode);
  bool GetResumePoint(CVideoInfoTag& tag);
  bool GetStreamDetails(CFileItem& item);
  bool GetStreamDetails(CVideoInfoTag& tag);
  bool GetStreamDetails(const std::string& filenameAndPath, CStreamDetails& details);
  /*! \brief Get requested details for a whole list at once, as the per-item loaders fill them. */
  void GetDetailsForList(const std::vector<CVideoInfoTag*>& tags,
                         KODI::MEDIA::TYPE media_type,
                         int getDetails);
  /*! \brief Get many items' stream details as GetStreamDetails() does; keeps m_parsedDetails. */
  void GetStreamDetailsForList(const std::vector<CVideoInfoTag*>& tags);
  /*!
   * \brief Add many items' stream details without picking best streams, which copying them does.
   * \return false if the database isn't open or the query failed, leaving the details incomplete.
   */
  bool AddStreamDetailsForList(const std::vector<CVideoInfoTag*>& tags);
  /*!
   * \brief Get the art of many items of one media type; items without art get no entry.
   * \return false if the database isn't open or the query failed, leaving the art incomplete.
   */
  bool GetArtForItems(const std::string& mediaType,
                      const std::vector<int>& ids,
                      std::map<int, KODI::ART::Artwork>& art);
  /*! \brief Get play count, last played, resume point and stream details of all files of a path
   Obtaining the metadata of many files one by one is expensive if the database connection has
   high latency, as every single value requires its own round trip.
   \param strPath the path to get the file metadata for
   \param metadata filled with a tag per file, keyed by file name
   \return true on success, false otherwise
   */
  bool GetFileMetadataForPath(const std::string& strPath,
                              std::map<std::string, CVideoInfoTag>& metadata);
  /*! \brief The details of the library item of \p type with \p id.
   \param item when given, filled as the library lists a set, show or season
   \param idVersion, idFile for a movie, the version to read, as TryGetMovieInfo takes them
   */
  GetResult TryGetDetailsByTypeAndId(KODI::MEDIA::TYPE type,
                                     int id,
                                     CVideoInfoTag& details,
                                     CFileItem* item = nullptr,
                                     int getDetails = VideoDbDetailsAll,
                                     int idVersion = -1,
                                     int idFile = -1);
  bool GetDetailsByTypeAndId(CFileItem& item, VideoDbContentType type, int id);
  CVideoInfoTag GetDetailsByTypeAndId(VideoDbContentType type, int id);

  //! \brief The details of the library item a tag is: for a movie, of the version or file it names
  bool GetDetailsOfItem(const CVideoInfoTag& item,
                        CVideoInfoTag& details,
                        int getDetails = VideoDbDetailsAll);

  //! \brief Rename a movie, set, TV show, season, episode or music video. A season keeps its name
  //! for an empty one, as saving its details does.
  bool SetItemTitle(const CVideoInfoTag& item, const std::string& title);

  /*! \name Content geometry
   * The measured picture rectangle of a file. Detection results only; a declared ratio
   * lives in the settings table.
   */
  ///@{

  //! \brief Store the content geometry of a file, replacing any previous one.
  bool SetContentGeometry(int idFile, const KODI::VIDEO::GEOMETRY::ContentGeometryRecord& geometry);

  //! \brief Look up the content geometry of a file, checking it still describes that file.
  //! MISSING both when there is none and when what is stored was measured from other content.
  KODI::VIDEO::GEOMETRY::ContentGeometryLookup GetContentGeometry(
      int idFile, const KODI::VIDEO::GEOMETRY::FileIdentity& identity);

  //! \brief GetContentGeometry() for the file at \p path, which is asked for its identity only
  //! when a measurement is stored to check it against.
  KODI::VIDEO::GEOMETRY::ContentGeometryLookup GetContentGeometryOfFile(int idFile,
                                                                        const std::string& path);

  //! \brief What is stored for a file, including a record that found nothing, without checking
  //! it still describes the file. Anything acting on the rectangle uses GetContentGeometry().
  std::optional<KODI::VIDEO::GEOMETRY::ContentGeometryRecord> GetStoredContentGeometry(int idFile);

  //! \brief Every file in the library with whatever content geometry is stored for it,
  //! unfiltered.
  std::vector<ContentGeometryCandidate> GetContentGeometryCandidates();

  ///@}

  // scraper settings
  struct StringHash
  {
    using is_transparent = void; // Enables heterogeneous operations.
    std::size_t operator()(std::string_view sv) const
    {
      std::hash<std::string_view> hasher;
      return hasher(sv);
    }
  };
  using ScraperCache =
      std::unordered_map<std::string, ADDON::ScraperPtr, StringHash, std::equal_to<>>;
  void SetScraperForPath(const std::string& filePath,
                         const ADDON::ScraperPtr& info,
                         const KODI::VIDEO::SScanSettings& settings);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath,
                                      ScraperCache* scraperCache = nullptr);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath,
                                      KODI::VIDEO::SScanSettings& settings,
                                      ScraperCache* scraperCache = nullptr);

  /*! \brief Retrieve the scraper and settings we should use for the specified path
   If the scraper is not set on this particular path, we'll recursively check parent folders.
   \param strPath path to start searching in.
   \param settings [out] scan settings for this folder.
   \param foundDirectly [out] true if a scraper was found directly for strPath, false if it was in a parent path.
   \return A ScraperPtr containing the scraper information. Returns nullptr if a trivial (Content == CONTENT_NONE)
           scraper or no scraper is found.
   */
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath,
                                      KODI::VIDEO::SScanSettings& settings,
                                      bool& foundDirectly,
                                      ScraperCache* scraperCache = nullptr);

  /*! \brief Determine whether the path is using lookup using folders: its source has its items in
   folders of their own, and it isn't in the source's root. A disc's is that of its file, not of its
   playlist.
   \param path the path to check
   \param shows whether this path is from a tvshow (defaults to false)
   \return true if items under the path are looked up by folder name
   */
  bool LookupByFolders(const std::string& path, bool shows = false);

  /*! \brief Retrieve the content type of videos in the given path
   If content is set on the folder, we return the given content type, except in the case of tvshows,
   where we first check for whether we have episodes directly in the path (thus return episodes) or whether
   we've found a scraper directly (shows).  Any folders inbetween are treated as seasons (regardless of whether
   they actually are seasons). Note that any subfolders in movies will be treated as movies.
   \param strPath path to start searching in.
   \return A content type string for the current path.
   */
  std::string GetContentForPath(const std::string& strPath);

  /*! \brief Get videos of the given content type from the given path
   \param content the content type to fetch.
   \param path the path to fetch videos from.
   \param items the returned items
   \param getDetails bitmask specifying which additional video details to load
   \return true if items are found, false otherwise.
   */
  bool GetItemsForPath(const std::string& content,
                       const std::string& path,
                       CFileItemList& items,
                       int getDetails = VideoDbDetailsNone);

  /*! \brief Check whether a given scraper is in use.
   \param scraperID the scraper to check for.
   \return true if the scraper is in use, false otherwise.
   */
  bool ScraperInUse(const std::string &scraperID) const;

  // scanning hashes and paths scanned
  bool SetPathHash(const std::string &path, const std::string &hash);
  bool GetPathHash(const std::string &path, std::string &hash);
  /*! \brief Clear the hash of the folder of a video's path, and of the folder above it
   where that is what names the video, so that the next scan looks at them again.
   */
  void InvalidatePathHash(const std::string& strPath);
  bool GetPaths(std::set<std::string, std::less<>>& paths);
  bool GetPathsForTvShow(int idShow, std::set<int>& paths);

  /*! \brief return the paths linked to a tvshow.
   \param idShow the id of the tvshow.
   \param paths [out] the list of paths associated with the show.
   \return true on success, false on failure.
   */
  bool GetPathsLinkedToTvShow(int idShow, std::vector<std::string> &paths);

  /*! \brief retrieve subpaths of a given path.  Assumes a hierarchical folder structure
   \param basepath the root path to retrieve subpaths for
   \param subpaths the returned subpaths
   \param excludeDiscPaths true - exclude disc paths that contain VIDEO_TS.IFO or INDEX.BDMV
                             (default)
                           false - include encoded paths for cleaning (eg. bluray://, zip:// etc.)
   \return true if we successfully retrieve subpaths (may be zero), false on error
   */
  bool GetSubPaths(const std::string& basepath,
                   std::vector<std::pair<int, std::string>>& subpaths,
                   bool excludeDiscPaths = true);

  /*! \brief Normalise a directory to the form the path table stores it in.
   \param directory the directory as it was given
   \return the same directory with platform separators and a trailing separator
   */
  static std::string ToStoredPath(const std::string& directory);

  /*! \brief The id column of the movie, tvshow, episode or musicvideo table, whichever holds
   \p type.
   \return empty for any other type
   */
  static std::string_view IdColumnOf(KODI::MEDIA::TYPE type);

  /*! \brief Resolve the path ids a library clean should cover.
   \param directory a directory to restrict the clean to, empty for the whole library.
                    Normalised with ToStoredPath before matching.
   \param content the content type to clean for ("movies", "tvshows", "musicvideos"),
                  empty for any. With a directory, "tvshows" also matches paths
                  resolving to "seasons" or "episodes".
   \param paths the matching path ids, including subpaths and the disc and archive paths
                below them. Left empty when nothing matches.
   \return true on success (even with no matches), false on a database error
   */
  bool GetPathsForCleaning(const std::string& directory,
                           const std::string& content,
                           std::set<int>& paths);

  bool GetSourcePath(const std::string &path, std::string &sourcePath);
  bool GetSourcePath(const std::string& path,
                     std::string& sourcePath,
                     KODI::VIDEO::SScanSettings& settings);

  // for music + musicvideo linkups - if no album and title given it will return the artist id, else the id of the matching video
  int GetMatchingMusicVideo(const std::string& strArtist, const std::string& strAlbum = "", const std::string& strTitle = "");

  // searching functions
  void GetMoviesByActor(const std::string& strActor, CFileItemList& items);
  void GetTvShowsByActor(const std::string& strActor, CFileItemList& items);
  void GetEpisodesByActor(const std::string& strActor, CFileItemList& items);

  void GetMusicVideosByArtist(const std::string& strArtist, CFileItemList& items);
  void GetMusicVideosByAlbum(const std::string& strAlbum, CFileItemList& items);

  void GetMovieGenresByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowGenresByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoGenresByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieCountriesByName(const std::string& strSearch, CFileItemList& items);

  void GetMusicVideoAlbumsByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieActorsByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsActorsByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoArtistsByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieDirectorsByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsDirectorsByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideoDirectorsByName(const std::string& strSearch, CFileItemList& items);

  void GetMoviesByName(const std::string& strSearch, CFileItemList& items);
  void GetTvShowsByName(const std::string& strSearch, CFileItemList& items);
  void GetEpisodesByName(const std::string& strSearch, CFileItemList& items);
  void GetMusicVideosByName(const std::string& strSearch, CFileItemList& items);

  void GetMovieExtrasByName(const std::string& strSearch, CFileItemList& items);

  std::string GetPlotByShowId(int idShow);
  void GetEpisodesByPlot(const std::string& strSearch, CFileItemList& items);
  void GetMoviesByPlot(const std::string& strSearch, CFileItemList& items);

  /*!
   * \brief Link or unlink a movie with a list of shows.
   * \param idMovie Id of the movie
   * \param shows List of show identifiers
   * \param remove true: remove link, false: add link
   * \return true for successful link/unlink of all shows, false on the first error otherwise.
   * \note Attempting to link an already linked show results in failure.
   */
  bool LinkMovieToTvshows(int idMovie, std::vector<int> shows, bool remove);
  bool LinkMovieToTvshow(int idMovie, int idShow, bool bRemove);
  bool IsLinkedToTvshow(int idMovie);
  bool GetLinksToTvShow(int idMovie, std::vector<int>& ids);

  // general browsing
  bool GetGenresNav(const std::string& strBaseDir,
                    CFileItemList& items,
                    VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                    const Filter& filter = Filter(),
                    bool countOnly = false);
  bool GetCountriesNav(const std::string& strBaseDir,
                       CFileItemList& items,
                       VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                       const Filter& filter = Filter(),
                       bool countOnly = false);
  bool GetStudiosNav(const std::string& strBaseDir,
                     CFileItemList& items,
                     VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                     const Filter& filter = Filter(),
                     bool countOnly = false);
  bool GetYearsNav(const std::string& strBaseDir,
                   CFileItemList& items,
                   VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                   const Filter& filter = Filter());
  bool GetActorsNav(const std::string& strBaseDir,
                    CFileItemList& items,
                    VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                    const Filter& filter = Filter(),
                    bool countOnly = false);
  bool GetDirectorsNav(const std::string& strBaseDir,
                       CFileItemList& items,
                       VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                       const Filter& filter = Filter(),
                       bool countOnly = false);
  bool GetWritersNav(const std::string& strBaseDir,
                     CFileItemList& items,
                     VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                     const Filter& filter = Filter(),
                     bool countOnly = false);
  bool GetSetsNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                  const Filter& filter = Filter(),
                  bool ignoreSingleMovieSets = false);
  bool GetTagsNav(const std::string& strBaseDir,
                  CFileItemList& items,
                  VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                  const Filter& filter = Filter(),
                  bool countOnly = false);

  bool GetMusicVideoAlbumsNav(const std::string& strBaseDir, CFileItemList& items, const Filter &filter = Filter(), bool countOnly = false);

  bool GetMoviesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idCountry=-1, int idSet=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetTvShowsNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idStudio=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSeasonsNav(const std::string& strBaseDir, CFileItemList& items, int idActor=-1, int idDirector=-1, int idGenre=-1, int idYear=-1, int idShow=-1, bool getLinkedMovies = true);
  bool GetEpisodesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idActor=-1, int idDirector=-1, int idShow=-1, int idSeason=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1, int idTag=-1, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);

  bool GetRecentlyAddedMoviesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetRecentlyAddedEpisodesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetRecentlyAddedMusicVideosNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit=0, int getDetails = VideoDbDetailsNone);
  bool GetInProgressTvShowsNav(const std::string& strBaseDir,
                               CFileItemList& items,
                               int getDetails = VideoDbDetailsNone);

  bool GetMoviesBySet(const std::string& baseDir, CFileItemList& items, int idSet);

  bool HasContent();
  bool HasContent(VideoDbContentType type);
  bool HasSets() const;

  void CleanDatabase(CGUIDialogProgressBarHandle* handle = nullptr,
                     const std::set<int>& paths = std::set<int>(),
                     bool showProgress = true);

  enum class FileExistsAction
  {
    ACTION_NONE,
    ACTION_UPDATE
  };

  int AddOrUpdateFile(const std::string& fileAndPath,
                      const std::string& parentPath,
                      const FileRecord& fileInfo,
                      FileExistsAction existsAction);

  /*! \brief Add a file to the database, if necessary
   If the file is already in the database, we simply return its id.
   \param url - full path of the file to add.
   \param parentPath the parent path of the path to add. If empty, URIUtils::GetParentPath() will determine the path.
   \param dateAdded datetime when the file was added to the filesystem/database
   \param playcount the playcount of the file to add.
   \param lastPlayed the date and time when the file to add was last played.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const std::string& url,
              const std::string& parentPath = "",
              const CDateTime& dateAdded = CDateTime(),
              int playcount = 0,
              const CDateTime& lastPlayed = CDateTime());

  /*! \brief Add a file to the database, if necessary
   Works for both videodb:// items and normal fileitems
   \param item CFileItem to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CFileItem& item);

  /*! \brief Add a file to the database, if necessary
   Works for both videodb:// items and normal fileitems
   \param url full path of the file to add.
   \param details details of the item to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const CVideoInfoTag& details, const std::string& parentPath = "");

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \param parentPath the parent path of the path to add. If empty, URIUtils::GetParentPath() will determine the path.
   \param dateAdded datetime when the path was added to the filesystem/database
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const std::string& strPath, const std::string &parentPath = "", const CDateTime& dateAdded = CDateTime());

  /*! \brief Updates the dateAdded field in the files table for the file
   with the given idFile and the given path based on the files modification date
   \param details details of the video file
   */
  void UpdateFileDateAdded(CVideoInfoTag& details);

  void ExportToXML(const std::string &path, bool singleFile = true, bool images=false, bool actorThumbs=false, bool overwrite=false);
  void ExportArt(const CFileItem& item,
                 const KODI::ART::Artwork& artwork,
                 KODI::MEDIA::TYPE mediaType,
                 bool overwrite) const;
  void ExportActorThumbs(const std::string& path,
                         const std::string& singlePath,
                         const CVideoInfoTag& tag,
                         bool singleFiles,
                         bool overwrite = false,
                         const std::string& tvshowDir = "") const;
  void ImportFromXML(const std::string &path);
  void DumpToDummyFiles(const std::string &path);
  bool ImportArtFromXML(const TiXmlNode* node, KODI::ART::Artwork& artwork) const;

  // smart playlists and main retrieval work in these functions
  bool GetMoviesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSetsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool ignoreSingleMovieSets = false);
  bool GetTvShowsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = VideoDbDetailsNone);
  bool GetSeasonsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath = true, const SortDescription &sortDescription = SortDescription());
  bool GetEpisodesByWhere(const std::string& strBaseDir,
                          const Filter& filter,
                          CFileItemList& items,
                          const SortDescription& sortDescription = SortDescription(),
                          int getDetails = VideoDbDetailsNone);
  bool GetMusicVideosByWhere(const std::string& baseDir,
                             const Filter& filter,
                             CFileItemList& items,
                             const SortDescription& sortDescription = SortDescription(),
                             int getDetails = VideoDbDetailsNone);

  // retrieve sorted and limited items
  bool GetSortedVideos(KODI::MEDIA::TYPE mediaType, const std::string& strBaseDir,
                       const SortDescription &sortDescription, CFileItemList& items,
                       const Filter &filter = Filter());

  // retrieve a list of items
  bool GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, const std::string &mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string& strBaseDir,
                VideoDbContentType mediaType,
                const std::string& itemType,
                CFileItemList& items,
                const Filter& filter = Filter(),
                const SortDescription& sortDescription = SortDescription());
  std::string GetItemById(const std::string &itemType, int id);

  // partymode
  /*! \brief Gets music video IDs in random order that match the where clause
  \param strWhere the SQL where clause to apply in the query
  \param songIDs a vector of <2, id> pairs suited to party mode use
  \return count of music video IDs found.
  */
  unsigned int GetRandomMusicVideoIDs(const std::string& strWhere, std::vector<std::pair<int, int> > &songIDs);

  void UpdateArtForItem(int mediaId, const std::string& mediaType) const;

  /*!
   * \brief Retrieve all art for the given video asset, with optional fallback to the art of the
   * parent/owner of the asset
   * \param assetId id of the file of the asset
   * \param fallback optionally request fallback to the art of the parent/owner for each art type
     that is not defined for the asset
   * \param art collection of the retrieved art
   * \return 
  */
  bool GetArtForAsset(int assetId, ArtFallbackOptions fallback, KODI::ART::Artwork& art);
  /*!
   * \brief Retrieve season information of a TV show.
   * \param[in] showId ID of the show
   * \param[out] seasons Map of the season information. Key = ID of the season, values = season number
   * \return true for success, false otherwise
   */
  bool GetTvShowSeasons(int showId, std::map<int, int> &seasons);
  /*!
   * \brief Retrieve season information of a TV show.
   * \param[in] showId ID of the show
   * \param[out] seasons Map of the season information. Key = season number, values = SeasonAttributes structure
   * \return true for success, false otherwise
   */
  bool GetTvShowSeasons(int showId, std::map<int, CVideoInfoTag::SeasonAttributes>& seasons);

  /*!
   * \brief Get the custom named season.
   * \param tvshowId The tv show id relative to the season.
   * \param seasonId The season id for which to search the named title.
   * \return The named title if found, otherwise empty.
   */
  std::string GetTvShowNamedSeasonById(int tvshowId, int seasonId) const;

  bool GetTvShowSeasonArt(int mediaId, KODI::ART::SeasonsArtwork& seasonArt);

  /*! \brief Fetch the distinct types of available-but-unassigned art held in the
  database for a specific media item.
  \param mediaId the id in the media table.
  \param mediaType the type of media, which corresponds to the table the item resides in.
  \return the types of art e.g. "thumb", "fanart", etc.
  */
  std::vector<std::string> GetAvailableArtTypesForItem(int mediaId, KODI::MEDIA::TYPE mediaType);

  /*! \brief Fetch the list of available-but-unassigned art URLs held in the
  database for a specific media item and art type.
  \param mediaId the id in the media table.
  \param mediaType corresponds to the table the item resides in.
  \param artType e.g. "thumb", "fanart", etc.
  \return list of URLs
  */
  std::vector<CScraperUrl::SUrlEntry> GetAvailableArtForItem(
    int mediaId, KODI::MEDIA::TYPE mediaType, const std::string& artType);

  int AddTag(const std::string &tag);

  //! \brief Whether a tag of this name is given to any item of a type
  bool HasTag(const std::string& name, KODI::MEDIA::TYPE type) const;
  void AddTagToItem(int idItem, int idTag, KODI::MEDIA::TYPE type);
  void RemoveTagFromItem(int idItem, int idTag, KODI::MEDIA::TYPE type);

  //! \brief Give a tag to items of a type, in one transaction
  bool AddTagToItems(int idTag, const std::vector<int>& idItems, KODI::MEDIA::TYPE type);

  //! \brief Take a tag from items of a type, in one transaction
  bool RemoveTagFromItems(int idTag, const std::vector<int>& idItems, KODI::MEDIA::TYPE type);

  /*!
   * \brief The movies, TV shows or music videos that have a tag, or those that don't
   * \param idTag the tag; all of them for none
   * \param tagged true for those with the tag, false for those without it
   */
  bool GetVideosForTag(KODI::MEDIA::TYPE type, int idTag, bool tagged, CFileItemList& items);
  void RemoveTagsFromItem(int idItem, KODI::MEDIA::TYPE type);

  bool GetFilter(CDbUrl &videoUrl, Filter &filter, SortDescription &sorting) override;

  /*! \brief Will check if the season exists and if that is not the case add it to the database.
  \param[in] showID The id of the show in question.
  \param[in] season The season number we want to add.
  \param[in] name Name of the season
  \param[in] plot Plot of the season
  \return The dbId of the season.
  */
  int AddSeason(int showID, int season, const std::string& name = "", const std::string& plot = "");

  /*! \brief Add a movie set, or update it if it already exists.
   \param[in] strSet the (possibly user defined) title of the set
   \param[in] strOverview the overview of the set
   \param[in] strOriginalSet the title of the set as given by the scraper. Used to identify an
              existing set, and defaults to strSet when empty.
   \param[in] strSortSet the title used to sort the set. An empty value never clears an existing
              sort title, so callers without one to offer can leave it out.
   \param[in] updateOverview whether an existing set's overview should be replaced
   \return the dbId of the set, or -1 on failure.
   */
  int AddSet(const std::string& strSet,
             const std::string& strOverview = "",
             const std::string& strOriginalSet = "",
             const std::string& strSortSet = "",
             const bool updateOverview = true);
  void ClearMovieSet(int idMovie);
  void SetMovieSet(int idMovie, int idSet);

  //! \brief AddSet, for a movie to be put in: a set with no art yet is given the movie's
  int AddSetForMovie(const std::string& name, int idMovie);
  bool SetVideoUserRating(int dbId, int rating, KODI::MEDIA::TYPE mediaType);
  bool GetUseAllExternalAudioForVideo(const std::string& videoPath);

  std::string GetSetByNameLike(const std::string& nameLike) const;

  std::string GetVideoVersionById(int id);
  int GetVideoVersionByTitle(const std::string& title, VideoAssetType itemType) const;
  void GetVideoVersions(VideoDbContentType itemType,
                        int dbId,
                        CFileItemList& items,
                        VideoAssetType videoAssetType);
  void GetDefaultVideoVersion(VideoDbContentType itemType, int dbId, CFileItem& item);

  /*!
   * \brief Remove a video from the library and transfer all of its assets to another video of the
   * same type.
   * \param itemType[in] Type of the video being converted
   * \param dbIdSource[in] id of the video being converted
   * \param dbIdTarget[in] id that the video will be attached to
   * \param idVideoVersion[in] new versiontype of the default version of the video
   *                           special value -1: keep the current versiontype of the video.
   * \param assetType[in] new asset type of the default version of the video.
   * \return true for success, false otherwise
   */
  bool ConvertVideoToVersion(VideoDbContentType itemType,
                             int dbIdSource,
                             int dbIdTarget,
                             int idVideoVersion,
                             VideoAssetType assetType);

  /*!
   * \brief Make a version of a movie a movie of its own: the movie's details, with the version's
   * file, name and art, linked to the movie's TV shows.
   * \param movie the movie the version belongs to
   * \param version the version, as Manage versions lists it
   * \return the new movie's id, -1 on failure
   */
  int ConvertVersionToVideo(const CVideoInfoTag& movie, const CVideoInfoTag& version);

  //! \brief Make a file the default version of a video. Outside a caller's transaction a change
  //! is announced.
  bool SetDefaultVideoVersion(VideoDbContentType itemType, int dbId, int idFile);
  void SetVideoVersion(int idFile, int idVideoVersion);
  int AddOrValidateVideoVersionType(const std::string& typeVideoVersion, VideoAssetType itemType);
  int AddVideoVersionType(const std::string& typeVideoVersion,
                          VideoAssetTypeOwner owner,
                          VideoAssetType assetType);
  /*!
   * \brief Create a new video asset from the provided item and type and attach it to an owner
   * A file record is created for items with a path new to the database. The file of another
   * movie's default version brings that movie, which becomes the asset with its own assets.
   * \param[in] itemType Parent's type
   * \param[in] dbId  Parent's id
   * \param[in] idVideoAsset Video asset identifier / name
   * \param[in] videoAssetType Type of the video asset
   * \param[in] item Item to be made into a video asset
   * \return Success status. true:success, false:failure, also where CanAddVideoAsset refuses
   * \sa RemoveVideoAsset
   */
  bool AddVideoAsset(VideoDbContentType itemType,
                     int dbId,
                     int idVideoAsset,
                     VideoAssetType videoAssetType,
                     CFileItem& item);

  /*!
   * \brief Whether a file can become another movie's version or extra: not the default version of
   * a movie with other versions, which would be left without a file of its own.
   */
  bool CanAddVideoAsset(int idFile);

  //! \brief Unlink a version or an extra from its movie, keeping its file and that file's state.
  //! \sa LinkVideoAsset, RemoveVideoAsset
  bool UnlinkVideoAsset(int idFile,
                        DeleteMovieHashAction hashAction = DeleteMovieHashAction::HASH_DELETE);

  /*!
   * \brief Remove a version or an extra from its movie. The file of one on a disc playlist goes too,
   * as nothing else lists it.
   * Outside a caller's transaction the movie's change is announced.
   * \return false for a movie's default version, or if it could not be removed
   * \sa AddVideoAsset
   */
  bool RemoveVideoAsset(int idFile);

  //! A movie or an episode RevertPlaylistsToDisc moved off a disc playlist onto the disc
  struct PlaylistRevert
  {
    CVideoInfoTag details; //!< as it is now
    int oldIdFile{-1};
    int playlist{-1};
  };

  enum class PlaylistRevertResult
  {
    DONE,
    IN_USE, //!< a playlist's file is still a version of another movie, so nothing changed
    FAILED,
  };

  /*!
   * \brief Take disc playlists from the library items they belong to. An extra on one goes; a movie
   * or an episode moves back to the disc, which keeps its own play history. Outside a caller's
   * transaction the changes, and the movies whose extras went, are announced.
   * \param disc the disc's file
   * \param reverted [out] the movies and episodes moved to the disc
   */
  PlaylistRevertResult RevertPlaylistsToDisc(
      const std::string& disc,
      const std::vector<KODI::VIDEO::DiscPlaylist>& playlists,
      std::vector<PlaylistRevert>& reverted);

  //! How MoveFileOfPlayedItem moved a played library item
  struct PlayedFileMove
  {
    int idFile{-1}; //!< the item's file now
    int replacedIdFile{-1}; //!< the file it stopped using, -1 if it kept it
    KODI::MEDIA::TYPE mediaType{KODI::MEDIA::TYPE::NONE};
    int idMedia{-1};
  };

  //! Whether MoveFileOfPlayedItem moves the played library item
  static bool PlayedItemMovesFile(const CFileItem& item);

  /*!
   * \brief Point a played library item at the file it was played from, where that changed: a disc
   * played by another playlist, or a stack played under a new stack path. A version or an extra
   * moves with its movie.
   * \param file the file the item's play state is kept against
   * \return nullopt if nothing needed moving, or the move failed
   */
  std::optional<PlayedFileMove> MoveFileOfPlayedItem(const CFileItem& item,
                                                     const std::string& file);
  bool IsDefaultVideoVersion(int idFile);
  bool GetVideoVersionTypes(VideoDbContentType idContent,
                            VideoAssetType assetType,
                            CFileItemList& items);

  /*!
   * \brief Check the validity of the video asset type id.
   * \param[in] typeId Id of the video asset type
   * \param[in] idContent db item type
   * \param[in] asset type of the video asset type
   * \return true when the id exists and matches the provided content and asset type, false otherwise.
   */
  bool IsValidVideoAssetType(int typeId, VideoDbContentType idContent, VideoAssetType asset);
  bool SetVideoVersionDefaultArt(int dbId, int idFrom, KODI::MEDIA::TYPE mediaType);
  void UpdateVideoVersionTypeTable();
  bool GetVideoVersionsNav(const std::string& strBaseDir,
                           CFileItemList& items,
                           VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                           const Filter& filter = Filter());
  VideoAssetInfo GetVideoVersionInfo(const std::string& filenameAndPath);
  bool UpdateAssetsOwner(KODI::MEDIA::TYPE mediaType, int dbIdSource, int dbIdTarget);

  int GetMovieId(const std::string& strFilenameAndPath);

  /*!
   \brief The movie whose files lie in a folder or below it, other than below excludedFolder.
   Files on a disc or in an archive in the folder count too.
   \return the movie, or -1 if there is none or more than one
   */
  int GetMovieIdInFolder(const std::string& folder, const std::string& excludedFolder);

  /*!
   \brief Work out the date a file, or a show's folder, is added to the library with, where nothing
   gives it one, before the transaction it is added in, so that the file system isn't asked while the
   database is held
   \param fileAndPath the file or folder, as it is added
   */
  void PrepareDateAdded(const std::string& fileAndPath);
  void ForgetPreparedDatesAdded();

  std::string GetMovieTitle(int idMovie);

  enum MatchingMask : uint8_t
  {
    None = 0x00,
    UniqueId = 0x01,
    Path = 0x02,
    Title = 0x04
  };

  void GetSameVideoItems(const CFileItem& item,
                         CFileItemList& items,
                         int matchingMask = UniqueId | Title);
  int GetFileIdByMovie(int idMovie);
  std::string GetFileBasePathById(int idFile);

  /*!
   * @brief Check the passed in list of images if used in this database. Used to clean the image cache.
   * @param imagesToCheck
   * @return a list of the passed in images used by this database.
   */
  std::vector<std::string> GetUsedImages(const std::vector<std::string>& imagesToCheck);

  /*! \brief Find a playlist path for a removable bluray disc.
   \param originalPath A path in the format of bluray://removable://<title_ID>/BDMV/index.bdmv
   \return A path in the format of bluray://removable://<title_ID>/BDMV/PLAYLIST/00000.mpls if found, otherwise an empty string.
   */
  std::string GetRemovableBlurayPath(std::string originalPath);

  /*!
   * \brief The resume point of a disc in a drive, by the id CMediaManager::GetDiskUniqueId gives.
   * A Blu-ray's is kept against the playlist last played from it.
   */
  bool GetDiscResumeBookMark(const std::string& discId, CBookmark& bookmark);

protected:
  //! \brief The play state a TV show's edit leaves one of its episodes with
  struct PlayState
  {
    int playCount;
    CDateTime lastPlayed;
  };

  //! \brief The play state \p showEdit leaves \p episode with, nullopt for it to be left as it is
  static std::optional<PlayState> EpisodePlayState(const PlayStateEdit& showEdit,
                                                   const CVideoInfoTag& episode);

  //! \brief Adds the play state a file's row holds to \p details, which say what the file is
  static void AddFilePlayState(const CVideoInfoTag& file, CVideoInfoTag& details);

  int AddNewMovie(CVideoInfoTag& details);
  int AddNewMusicVideo(CVideoInfoTag& details);

  int GetMusicVideoId(const std::string& strFilenameAndPath);

  int GetFileId(const CVideoInfoTag& details);

  /*! \brief Get the id of the file of this item and store it in the item
   \param details CVideoInfoTag for which to get and store the id of the file
   \return id of the file, -1 if it is not in the db.
   */
  int GetAndFillFileId(CVideoInfoTag& details);

  /*! \brief Get the id of a file from path
   \param url full path to the file
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const std::string& url);

  /*! \brief Get the id of a stack of discs, however its parts are expressed.
   A stack holding a bluray folders or disc images may be stored resolved to the
   bluray:// playlist of each part, whereas the scraper will be looking for a
   stack:// of the base paths.
   \param stackPath a stack:// path of which at least one part is a disc
   \return id of the file, -1 if it is not in the db or several stacks match.
   */
  int GetDiscStackFileId(const std::string& stackPath);

  int AddToTable(const std::string& table, const std::string& firstField, const std::string& secondField, const std::string& value);
  int UpdateRatings(int mediaId, KODI::MEDIA::TYPE mediaType, const RatingMap& values,
                    const std::string& defaultRating);
  int AddRatings(int mediaId,
                 KODI::MEDIA::TYPE mediaType,
                 const RatingMap& values,
                 std::string_view defaultRating);
  int UpdateUniqueIDs(int mediaId, KODI::MEDIA::TYPE mediaType, const CVideoInfoTag& details);
  int AddUniqueIDs(int mediaId, KODI::MEDIA::TYPE mediaType, const CVideoInfoTag& details);
  int AddActor(const std::string& strActor, const std::string& thumbURL, const std::string &thumb = "");

  int AddTvShow();

  /*! \brief Adds a path to the tvshow link table.
   \param idShow the id of the show.
   \param path the path to add.
   \param dateAdded date/time when the path was added
   \return true if successfully added, false otherwise.
   */
  bool AddPathToTvShow(int idShow, const std::string& path, const CDateTime& dateAdded);

  /*! \brief Check whether a show is already in the library.
   Matches on unique identifier or matching title and premiered date.
   \param show the details of the show to check for.
   \return the show id if found, else -1.
   */
  int GetMatchingTvShow(const CVideoInfoTag& show) const;

  // link functions - these two do all the work
  void AddLinkToActor(int mediaId,
                      KODI::MEDIA::TYPE mediaType, int actorId, const std::string &role, int order);
  void AddToLinkTable(int mediaId,
                      KODI::MEDIA::TYPE mediaType,
                      const std::string& table,
                      int valueId,
                      const char* foreignKey = nullptr);
  void RemoveFromLinkTable(int mediaId,
                           KODI::MEDIA::TYPE mediaType,
                           const std::string& table,
                           int valueId,
                           const char* foreignKey = nullptr);

  void AddLinksToItem(int mediaId, KODI::MEDIA::TYPE mediaType, const std::string& field,
                      const std::vector<std::string>& values);
  void UpdateLinksToItem(int mediaId, KODI::MEDIA::TYPE mediaType, const std::string& field,
                         const std::vector<std::string>& values);
  void AddActorLinksToItem(int mediaId, KODI::MEDIA::TYPE mediaType, const std::string& field,
                           const std::vector<std::string>& values);
  void UpdateActorLinksToItem(int mediaId, KODI::MEDIA::TYPE mediaType, const std::string& field,
                              const std::vector<std::string>& values);

  void AddCast(int mediaId, KODI::MEDIA::TYPE mediaType, const std::vector<SActorInfo> &cast);

  //! \brief Builds the item of a listing's row and adds it, unless the profile may not list it
  //! \return the item's tag, for the details the list loads once; nullptr for no item
  using ListRow = std::function<CVideoInfoTag*(
      const dbiplus::sql_record* record, int details, CFileItemList& items)>;
  //! \brief Makes a listing's row builder, once the listing's url is known
  using ListRows = std::function<ListRow(const CVideoDbUrl& videoUrl)>;

  /*!
   * \brief List the movies, TV shows, episodes or music videos a videodb url and a filter select,
   * sorted and limited, each row built by the type's \p rows
   */
  bool ListVideos(KODI::MEDIA::TYPE type,
                  const std::string& baseDir,
                  const Filter& filter,
                  CFileItemList& items,
                  const SortDescription& sortDescription,
                  int getDetails,
                  const ListRows& rows);

  //! \brief Whether the profile may list an item at \p path
  bool IsUnlocked(const std::string& path) const;

  CVideoInfoTag GetDetailsForMovie(dbiplus::Dataset& pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMovie(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  CSetInfoTag GetDetailsForSet(dbiplus::Dataset& pDS) const;
  CSetInfoTag GetDetailsForSet(const dbiplus::sql_record* const record) const;
  CVideoInfoTag GetDetailsForTvShow(dbiplus::Dataset& pDS,
                                    int getDetails = VideoDbDetailsNone,
                                    CFileItem* item = nullptr);
  CVideoInfoTag GetDetailsForTvShow(const dbiplus::sql_record* const record,
                                    int getDetails = VideoDbDetailsNone,
                                    CFileItem* item = nullptr);
  CVideoInfoTag GetBasicDetailsForEpisode(dbiplus::Dataset& pDS) const;
  CVideoInfoTag GetBasicDetailsForEpisode(const dbiplus::sql_record* const record) const;
  CVideoInfoTag GetDetailsForEpisode(dbiplus::Dataset& pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForEpisode(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMusicVideo(dbiplus::Dataset& pDS, int getDetails = VideoDbDetailsNone);
  CVideoInfoTag GetDetailsForMusicVideo(const dbiplus::sql_record* const record, int getDetails = VideoDbDetailsNone);
  bool GetPeopleNav(const std::string& strBaseDir,
                    CFileItemList& items,
                    const char* type,
                    VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                    const Filter& filter = Filter(),
                    bool countOnly = false);
  bool GetNavCommon(const std::string& strBaseDir,
                    CFileItemList& items,
                    const char* type,
                    VideoDbContentType idContent = VideoDbContentType::UNKNOWN,
                    const Filter& filter = Filter(),
                    bool countOnly = false);
  void GetCast(int media_id, KODI::MEDIA::TYPE media_type, std::vector<SActorInfo> &cast);
  void GetTags(int media_id, KODI::MEDIA::TYPE media_type, std::vector<std::string> &tags);
  void GetRatings(int media_id, KODI::MEDIA::TYPE media_type, RatingMap &ratings);
  void GetUniqueIDs(int media_id, KODI::MEDIA::TYPE media_type, CVideoInfoTag& details);

  template<typename T>
  void GetDetailsFromDB(const dbiplus::sql_record* const record,
                        int min,
                        int max,
                        const T& offsets,
                        CVideoInfoTag& details,
                        int idxOffset = 2) const;
  template<typename T>
  void GetDetailsFromDB(const dbiplus::sql_record* const record,
                        int min,
                        int max,
                        const T& offsets,
                        CSetInfoTag& details,
                        int idxOffset) const;

  template<typename T>
  std::string GetValueString(const CVideoInfoTag& details,
                             int min,
                             int max,
                             const T& offsets) const;

  int SetFileForEpisode(const std::string& fileAndPath,
                        int idEpisode,
                        int oldIdFile,
                        int newIdFile);
  int SetFileForMovie(const std::string& fileAndPath, int idMovie, int oldIdFile, int newIdFile);
  int SetFileForUnknown(const std::string& fileAndPath, int oldIdFile, int newIdFile);

private:
  //! \brief Announce an update for an item named by its stored media type, which an actor has
  //! but no KODI::MEDIA::TYPE names.
  static void AnnounceUpdate(const std::string& mediaType, int id);

  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;

  /*!
   * \brief Link a file to a movie as a version or an extra, or change the link it has.
   * \param itemType type of the video the version belongs to
   * \param dbIdSource id of the video the version belongs to
   * \param idFile file id of the version
   * \param idVideoVersion versiontype of the version
   * \param assetType asset type of the version
   * \return true if success, false otherwise
   * \sa UnlinkVideoAsset
   */
  bool LinkVideoAsset(VideoDbContentType itemType,
                      int dbIdSource,
                      int idFile,
                      int idVideoVersion,
                      VideoAssetType assetType);

  //! \brief Fill details for a version or extra of owner, as its asset list shows it
  bool GetVideoAssetInfo(int idFile,
                         int idType,
                         const std::string& name,
                         VideoAssetType type,
                         const CVideoInfoTag& owner,
                         CVideoInfoTag& details);

  //! \brief Announce a library item's change, clearing the cached listings first, unless updates
  //! are not announced
  void AnnounceItemChanged(KODI::MEDIA::TYPE type, int id);

  //! \brief SetPlayCount, without announcing the count
  CDateTime WritePlayCount(const CFileItem& item, int count, const CDateTime& date);

  bool m_announceFileChanges{true};

  /*! \brief Helper to get a database id given a query.
   Returns an integer, -1 if not found, and greater than 0 if found.
   \param query the SQL that will retrieve a database id.
   \return -1 if not found, else a valid database id (i.e. > 0)
   */
  int GetDbId(const std::string& query) const;

  /*! \brief Run a query on the main dataset and return the number of rows
   If no rows are found we close the dataset and return 0.
   \param sql the sql query to run
   \return the number of rows, -1 for an error.
   */
  int RunQuery(const std::string &sql);

  void AppendIdLinkFilter(const char* field,
                          const char* table,
                          KODI::MEDIA::TYPE mediaType,
                          const char* view,
                          const char* viewKey,
                          const CUrlOptions::UrlOptions& options,
                          Filter& filter) const;
  void AppendLinkFilter(const char* field,
                        const char* table,
                        KODI::MEDIA::TYPE mediaType,
                        const char* view,
                        const char* viewKey,
                        const CUrlOptions::UrlOptions& options,
                        Filter& filter) const;

  /*! \brief Get the playcount for a file id
   \param iFileId file id to get the playcount for
   \return the playcount of the item, or -1 on error
   \sa SetPlayCount, IncrementPlayCount, GetPlayCounts
   */
  int GetPlayCount(int iFileId);

  /*! \brief Get the last played time of a filename and path
   \param iFileId file id to get the playcount for
   \return the last played time of the item, or an invalid CDateTime on error
   \sa UpdateLastPlayed
   */
  CDateTime GetLastPlayed(int iFileId);

  /*! \brief Add the stream described by the current row of the given dataset to the given details
   \param ds dataset whose current row starts with the columns of the streamdetails table
   \param details stream details to add the stream to
   \return true if a stream was added, false otherwise
   */
  static bool AddStreamDetailFromRow(dbiplus::Dataset& ds, CStreamDetails& details);

  int GetMinSchemaVersion() const override { return 75; }
  int GetSchemaVersion() const override;
  virtual int GetExportVersion() const { return 1; }
  const char* GetBaseDBName() const override { return "MyVideos"; }

  void ConstructPath(std::string& strDest,
                     const std::string& strPath,
                     const std::string& strFileName) const;
  void SplitPath(const std::string& strFileNameAndPath,
                 std::string& strPath,
                 std::string& strFileName) const;

  /*! \brief Clear the hash of a path, without adding it to the path table if unknown
   \param strPath the path to clear the hash of
   */
  void ClearPathHash(const std::string& strPath);

  /*! \brief Get a safe filename from a given string
   \param dir directory to use for the file
   \param name movie, show name, or actor to get a safe filename for
   \return safe filename based on this title
   */
  std::string GetSafeFile(const std::string &dir, const std::string &name) const;

  std::vector<int> CleanMediaType(KODI::MEDIA::TYPE mediaType, const std::string &cleanableFileIDs,
                                  std::map<int, bool> &pathsDeleteDecisions, std::string &deletedFileIDs, bool silent);

  static void AnnounceRemove(KODI::MEDIA::TYPE content, int id, bool scanning = false);

  static CDateTime GetDateAdded(const std::string& filename, CDateTime dateAdded = CDateTime());

  //! \brief Store \p edit on a file, against the play state it holds
  bool StorePlayStateEdit(const std::string& file, const PlayStateEdit& edit);

  //! \brief Give each episode of a show the play state its edit sets
  bool StoreEpisodesPlayState(int idShow, const PlayStateEdit& showEdit);

  //! \brief A TV show that \p path is one of the folders of; a folder beneath one is not
  bool GetTvShowOfFolder(const std::string& path, CVideoInfoTag& details, CFileItem* item);

  //! The dates added of files and folders about to be added \sa PrepareDateAdded
  std::map<std::string, CDateTime, std::less<>> m_preparedDatesAdded;
  //! \brief The date \p path is added with: \p dateAdded, else the one prepared, else read now
  CDateTime DateAddedOf(const std::string& path, const CDateTime& dateAdded) const;
};
