/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "utils/EmbeddedArt.h"
#include "utils/Fanart.h"
#include "utils/ISortable.h"
#include "utils/ScraperUrl.h"
#include "utils/StreamDetails.h"
#include "video/Bookmark.h"

#include <string>
#include <vector>

class CArchive;
class TiXmlNode;
class TiXmlElement;
class CVariant;

enum class VideoAssetType;

struct SActorInfo
{
  bool operator<(const SActorInfo &right) const
  {
    return order < right.order;
  }
  std::string strName;
  std::string strRole;
  CScraperUrl thumbUrl;
  std::string thumb;
  int        order = -1;
};

class CRating
{
public:
  CRating() = default;
  explicit CRating(float r): rating(r) {}
  CRating(float r, int v): rating(r), votes(v) {}
  float rating = 0.0f;
  int votes = 0;
};
typedef std::map<std::string, CRating> RatingMap;

class CVideoInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CVideoInfoTag() { Reset(); }
  virtual ~CVideoInfoTag() = default;
  void Reset();
  /* \brief Load information to a videoinfotag from an XML element
   There are three types of tags supported:
    1. Single-value tags, such as <title>.  These are set if available, else are left untouched.
    2. Additive tags, such as <set> or <genre>.  These are appended to or replaced (if available) based on the value
       of the prioritise parameter.  In addition, a clear attribute is available in the XML to clear the current value prior
       to appending.
    3. Image tags such as <thumb> and <fanart>.  If the prioritise value is specified, any additional values are prepended
       to the existing values.

   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.

   \sa ParseNative
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, bool savePathInfo = true, const TiXmlElement *additionalNode = NULL);
  void Merge(CVideoInfoTag& other);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem& sortable, Field field) const override;
  const CRating GetRating(std::string type = "") const;
  const std::string& GetDefaultRating() const;
  const std::string GetUniqueID(std::string type = "") const;
  const std::map<std::string, std::string>& GetUniqueIDs() const;
  const std::string& GetDefaultUniqueID() const;
  bool HasUniqueID() const;
  virtual bool HasYear() const;
  virtual int GetYear() const;
  bool HasPremiered() const;
  const CDateTime& GetPremiered() const;
  const CDateTime& GetFirstAired() const;
  const std::string GetCast(bool bIncludeRole = false) const;
  bool HasStreamDetails() const;
  bool IsEmpty() const;

  const std::string& GetPath() const
  {
    if (m_strFileNameAndPath.empty())
      return m_strPath;
    return m_strFileNameAndPath;
  };

  /*! \brief set the duration in seconds
   \param duration the duration to set
   */
  void SetDuration(int duration);

  /*! \brief retrieve the duration in seconds.
   Prefers the duration from stream details if available.
   */
  unsigned int GetDuration() const;

  /*! \brief retrieve the duration in seconds.
   Ignores the duration from stream details even if available.
   */
  unsigned int GetStaticDuration() const;

  /*! \brief get the duration in seconds from a minute string
   \param runtime the runtime string from a scraper or similar
   \return the time in seconds, if decipherable.
   */
  static unsigned int GetDurationFromMinuteString(const std::string &runtime);

  void SetBasePath(std::string basePath);
  void SetDirector(std::vector<std::string> director);
  void SetWritingCredits(std::vector<std::string> writingCredits);
  void SetGenre(std::vector<std::string> genre);
  void SetCountry(std::vector<std::string> country);
  void SetTagLine(std::string tagLine);
  void SetPlotOutline(std::string plotOutline);
  void SetTrailer(std::string trailer);
  void SetPlot(std::string plot);
  std::string const& GetTitle() const;
  void SetTitle(std::string title);
  void SetSortTitle(std::string sortTitle);
  void SetPictureURL(CScraperUrl &pictureURL);
  void SetRating(float rating, int votes, const std::string& type = "", bool def = false);
  void SetRating(CRating rating, const std::string& type = "", bool def = false);
  void SetRating(float rating, const std::string& type = "", bool def = false);
  void RemoveRating(const std::string& type);
  void SetRatings(RatingMap ratings, const std::string& defaultRating = "");
  void SetVotes(int votes, const std::string& type = "");
  void SetUniqueIDs(std::map<std::string, std::string> uniqueIDs);
  void SetPremiered(const CDateTime& premiered);
  void SetPremieredFromDBDate(const std::string& premieredString);
  virtual void SetYear(int year);
  void SetArtist(std::vector<std::string> artist);
  void SetSet(std::string set);
  void SetSetOverview(std::string setOverview);
  void SetTags(std::vector<std::string> tags);
  void SetFile(std::string file);
  void SetPath(std::string path);
  void SetMPAARating(std::string mpaaRating);
  void SetFileNameAndPath(std::string fileNameAndPath);
  void SetOriginalTitle(std::string originalTitle);
  void SetEpisodeGuide(std::string episodeGuide);
  void SetStatus(std::string status);
  void SetProductionCode(std::string productionCode);
  void SetShowTitle(std::string showTitle);
  void SetStudio(std::vector<std::string> studio);
  void SetAlbum(std::string album);
  void SetShowLink(std::vector<std::string> showLink);
  void SetUniqueID(const std::string& uniqueid, const std::string& type = "", bool def = false);
  void RemoveUniqueID(const std::string& type);
  void SetNamedSeasons(std::map<int, std::string> namedSeasons);
  void SetUserrating(int userrating);

  /*!
   * @brief Get this videos's play count.
   * @return the play count.
   */
  virtual int GetPlayCount() const;

  /*!
   * @brief Set this videos's play count.
   * @param count play count.
   * @return True if play count was set successfully, false otherwise.
   */
  virtual bool SetPlayCount(int count);

  /*!
   * @brief Increment this videos's play count.
   * @return True if play count was increased successfully, false otherwise.
   */
  virtual bool IncrementPlayCount();

  /*!
   * @brief Reset playcount
   */
  virtual void ResetPlayCount();

  /*!
   * @brief Check if the playcount is set
   * @return True if play count value is set
   */
  virtual bool IsPlayCountSet() const;

  /*!
   * @brief Get this videos's resume point.
   * @return the resume point.
   */
  virtual CBookmark GetResumePoint() const;

  /*!
   * @brief Set this videos's resume point.
   * @param resumePoint resume point.
   * @return True if resume point was set successfully, false otherwise.
   */
  virtual bool SetResumePoint(const CBookmark &resumePoint);

  class CAssetInfo
  {
  public:
    /*!
     * @brief Clear all data.
     */
    void Clear();

    /*!
     * @brief Archive all data.
     * @param ar The archive to write the data to / to read the data from.
     */
    void Archive(CArchive& ar);

    /*!
     * @brief Store all data to XML.
     * @param movie The XML element to write the data to.
     */
    void Save(TiXmlNode* movie);

    /*!
     * @brief Restore all data from XML.
     * @param movie The XML element containing the data.
     */
    void ParseNative(const TiXmlElement* movie);

    /*!
     * @brief Merge in all valid data from another asset info.
     * @param other The other asset info.
     */
    void Merge(CAssetInfo& other);

    /*!
     * @brief Serialize all data.
     * @param value The container to write the data to.
     */
    void Serialize(CVariant& value) const;

    /*!
     * @brief Get the video's asset title.
     * @return The title or an empty string if the item has no video asset.
     */
    const std::string& GetTitle() const { return m_title; }

    /*!
     * @brief Set this videos's asset title.
     * @param assetTitle The title.
     */
    void SetTitle(const std::string& assetTitle);

    /*!
     * @brief Get the video's asset id.
     * @return The id or -1 if the item has no video asset.
     */
    int GetId() const { return m_id; }

    /*!
     * @brief Set this videos's asset id.
     * @param assetId The id.
     */
    void SetId(int assetId);

    /*!
     * @brief Get the video's asset type.
     * @return The type or VideoAssetType::UNKNOWN if the item has no video asset.
     */
    VideoAssetType GetType() const { return m_type; }

    /*!
     * @brief Set this videos's asset type.
     * @param assetType The type.
     */
    void SetType(VideoAssetType assetType);

  private:
    std::string m_title;
    int m_id{-1};
    VideoAssetType m_type{-1};
  };

  /*!
   * @brief Get the video's asset info.
   * @return The info.
   */
  const CAssetInfo& GetAssetInfo() const { return m_assetInfo; }
  CAssetInfo& GetAssetInfo() { return m_assetInfo; }

  /*!
   * @brief Whether the item has multiple video versions.
   * @return True if the item has multiple video versions, false otherwise.
   */
  bool HasVideoVersions() const { return m_hasVideoVersions; }

  /*!
   * @brief Set whether this video has video versions.
   * @param hasVersion The versions flag.
   */
  void SetHasVideoVersions(bool hasVersions);

  /*!
   * @brief Whether the item has video extras.
   * @return True if the item has video extras, false otherwise.
   */
  bool HasVideoExtras() const { return m_hasVideoExtras; }

  /*!
   * @brief Set whether this video has video extras.
   * @param hasExtras The extras flag.
   */
  void SetHasVideoExtras(bool hasExtras);

  /*!
   * @brief Whether the item is the default video version.
   * @return True if the item is the default version, false otherwise.
   */
  bool IsDefaultVideoVersion() const { return m_isDefaultVideoVersion; }

  /*!
   * @brief Set whether the item is the default version.
   * @param isDefaultVideoVersion The default flag.
   */
  void SetIsDefaultVideoVersion(bool isDefaultVideoVersion);

  /*!
  * @brief Get whether the Set Overview should be updated. If an NFO contains a <name> but no <overview> then
  * this allows the current Overview to be kept. Otherwise it is overwritten. Default is true - so if updated
  * by a scraper the Overview will be overwritten.
  */
  bool GetUpdateSetOverview() const { return m_updateSetOverview; }

  /*!
   * @brief Set this videos's resume point.
   * @param timeInSeconds the time of the resume point
   * @param totalTimeInSeconds the total time of the video
   * @param playerState the player state
   * @return True if resume point was set successfully, false otherwise.
   */
  virtual bool SetResumePoint(double timeInSeconds, double totalTimeInSeconds, const std::string &playerState);

  std::string m_basePath; // the base path of the video, for folder-based lookups
  int m_parentPathID;      // the parent path id where the base path of the video lies
  std::vector<std::string> m_director;
  std::vector<std::string> m_writingCredits;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_country;
  std::string m_strTagLine;
  std::string m_strPlotOutline;
  std::string m_strTrailer;
  std::string m_strPlot;
  CScraperUrl m_strPictureURL;
  std::string m_strTitle;
  std::string m_strSortTitle;
  std::vector<std::string> m_artist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;
  struct SetInfo //!< Struct holding information about a movie set
  {
    std::string title; //!< Title of the movie set
    int id; //!< ID of movie set in database
    std::string overview; //!< Overview/description of the movie set
  };
  SetInfo m_set; //!< Assigned movie set
  std::vector<std::string> m_tags;
  std::string m_strFile;
  std::string m_strPath;
  std::string m_strMPAARating;
  std::string m_strFileNameAndPath;
  std::string m_strOriginalTitle;
  std::string m_strEpisodeGuide;
  CDateTime m_premiered;
  bool m_bHasPremiered;
  std::string m_strStatus;
  std::string m_strProductionCode;
  CDateTime m_firstAired;
  std::string m_strShowTitle;
  std::vector<std::string> m_studio;
  std::string m_strAlbum;
  CDateTime m_lastPlayed;
  std::vector<std::string> m_showLink;
  std::map<int, std::string> m_namedSeasons;
  int m_iTop250;
  int m_year;
  int m_iSeason;
  int m_iEpisode;
  int m_iIdUniqueID;
  int m_iDbId;
  int m_iFileId;
  int m_iSpecialSortSeason;
  int m_iSpecialSortEpisode;
  int m_iTrack;
  RatingMap m_ratings;
  int m_iIdRating;
  int m_iUserRating;
  CBookmark m_EpBookmark;
  int m_iBookmarkId;
  int m_iIdShow;
  int m_iIdSeason;
  CFanart m_fanart;
  CStreamDetails m_streamDetails;
  CDateTime m_dateAdded;
  MediaType m_type;
  int m_relevance; // Used for actors' number of appearances
  int m_parsedDetails;
  std::vector<EmbeddedArtInfo> m_coverArt; ///< art information

  // TODO: cannot be private, because of 'struct SDbTableOffsets'
  unsigned int m_duration; ///< duration in seconds

private:
  /* \brief Parse our native XML format for video info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);

  std::string m_strDefaultRating;
  std::string m_strDefaultUniqueID;
  std::map<std::string, std::string> m_uniqueIDs;
  std::string Trim(std::string &&value);
  std::vector<std::string> Trim(std::vector<std::string> &&items);

  int m_playCount;
  CBookmark m_resumePoint;
  static const int PLAYCOUNT_NOT_SET = -1;

  CAssetInfo m_assetInfo;
  bool m_hasVideoVersions{false};
  bool m_hasVideoExtras{false};
  bool m_isDefaultVideoVersion{false};

  bool m_updateSetOverview{true};
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
