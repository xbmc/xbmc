/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_media.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_providers.h"
#include "pvr/PVRCachedImage.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "video/Bookmark.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>
#include <vector>

class CVideoDatabase;

namespace EDL
{
struct Edit;
}

struct PVR_MEDIA_TAG;

namespace PVR
{
class CPVRChannel;
class CPVRClient;
class CPVRProvider;
class CPVRTimerInfoTag;

/*!
   * @brief Representation of a CPVRMediaTag unique ID.
   */
class CPVRMediaTagUid final
{
public:
  int m_iClientId; /*!< ID of the backend */
  std::string m_strMediaTagId; /*!< unique ID of the media tag on the client */

  CPVRMediaTagUid(int iClientId, const std::string& strMediaTagId);

  bool operator>(const CPVRMediaTagUid& right) const;
  bool operator<(const CPVRMediaTagUid& right) const;
  bool operator==(const CPVRMediaTagUid& right) const;
  bool operator!=(const CPVRMediaTagUid& right) const;
};

class CPVRMediaTag final : public CVideoInfoTag
{
public:
  static const std::string IMAGE_OWNER_PATTERN;

  CPVRMediaTag();
  CPVRMediaTag(const PVR_MEDIA_TAG& mediaTag, unsigned int iClientId);

  bool operator==(const CPVRMediaTag& right) const;
  bool operator!=(const CPVRMediaTag& right) const;

  void Serialize(CVariant& value) const override;

  // ISortable implementation
  void ToSortable(SortItem& sortable, Field field) const override;

  /*!
   * @brief Reset this tag to it's initial state.
   */
  void Reset();

  /*!
   * @brief Set this media tag's play count. The value will be transferred to the backend if it supports server-side play counts.
   * @param count play count.
   * @return True if play count was set successfully, false otherwise.
   */
  bool SetPlayCount(int count) override;

  /*!
   * @brief Increment this media tag's play count. The value will be transferred to the backend if it supports server-side play counts.
   * @return True if play count was increased successfully, false otherwise.
   */
  bool IncrementPlayCount() override;

  /*!
   * @brief Set this media tag's play count without transferring the value to the backend, even if it supports server-side play counts.
   * @param count play count.
   * @return True if play count was set successfully, false otherwise.
   */
  bool SetLocalPlayCount(int count) { return CVideoInfoTag::SetPlayCount(count); }

  /*!
   * @brief Get this media tag's local play count. The value will not be obtained from the backend, even if it supports server-side play counts.
   * @return the play count.
   */
  int GetLocalPlayCount() const { return CVideoInfoTag::GetPlayCount(); }

  /*!
   * @brief Set this media tag's resume point. The value will be transferred to the backend if it supports server-side resume points.
   * @param resumePoint resume point.
   * @return True if resume point was set successfully, false otherwise.
   */
  bool SetResumePoint(const CBookmark& resumePoint) override;

  /*!
   * @brief Set this media tag's resume point. The value will be transferred to the backend if it supports server-side resume points.
   * @param timeInSeconds the time of the resume point
   * @param totalTimeInSeconds the total time of the video
   * @param playerState the player state
   * @return True if resume point was set successfully, false otherwise.
   */
  bool SetResumePoint(double timeInSeconds,
                      double totalTimeInSeconds,
                      const std::string& playerState = "") override;

  /*!
   * @brief Get this media tag's resume point. The value will be obtained from the backend if it supports server-side resume points.
   * @return the resume point.
   */
  CBookmark GetResumePoint() const override;

  /*!
   * @brief Get this media tag's local resume point. The value will not be obtained from the backend even if it supports server-side resume points.
   * @return the resume point.
   */
  CBookmark GetLocalResumePoint() const { return CVideoInfoTag::GetResumePoint(); }

  /*!
   * @brief Retrieve the edit decision list (EDL) of a media tag on the backend.
   * @return The edit decision list (empty on error)
   */
  std::vector<EDL::Edit> GetEdl() const;

  /*!
   * @brief Get the resume point and play count from the database if the
   * client doesn't handle it itself.
   * @param db The database to read the data from.
   * @param client The client this media tag belongs to.
   */
  void UpdateMetadata(CVideoDatabase& db, const CPVRClient& client);

  /*!
   * @brief Update this tag with the contents of the given tag.
   * @param tag The new tag info.
   * @param client The client this media tag belongs to.
   */
  void Update(const CPVRMediaTag& tag, const CPVRClient& client);

  /*!
   * @brief Retrieve the media tag start as UTC time
   * @return the media tag start time
   */
  const CDateTime& MediaTagTimeAsUTC() const { return m_mediaTagTime; }

  /*!
   * @brief Retrieve the media tag start as local time
   * @return the media tag start time
   */
  const CDateTime& MediaTagTimeAsLocalTime() const;

  /*!
   * @brief Retrieve the media tag end as UTC time
   * @return the media tag end time
   */
  CDateTime EndTimeAsUTC() const;

  /*!
   * @brief Retrieve the media tag end as local time
   * @return the media tag end time
   */
  CDateTime EndTimeAsLocalTime() const;

  /*!
   * @brief Retrieve the media tag title from the URL path
   * @param url the URL for the media tag
   * @return Title of the media tag
   */
  static std::string GetTitleFromURL(const std::string& url);

  /*!
   * @brief Check whether this is a tv or radio media tag
   * @return true if this is a radio media tag, false if this is a tv media tag
   */
  bool IsRadio() const { return m_bRadio; }

  /*!
   * @return The type of media stored in this tag defined by PVR_MEDIA_TAG_TYPE
   */
  PVR_MEDIA_TAG_TYPE MediaTagType() const { return m_mediaType; }

  /*!
   * @brief the identifier of the client that serves this media tag
   * @return the client identifier
   */
  int ClientID() const;

  /*!
   * @brief Get the media tag ID as upplied by the client
   * @return the media tag identifier
   */
  const std::string& ClientMediaTagID() const { return m_strMediaTagId; }

  /*!
   * @brief Get the media tag ID as upplied by the client
   * @return the media tag identifier
   */
  unsigned int MediaTagID() const { return m_iMediaTagId; }

  /*!
   * @brief Set the media tag ID
   * @param mediaTagId The new identifier
   */
  void SetMediaTagID(unsigned int mediaTagId) { m_iMediaTagId = mediaTagId; }

  /*!
   * @brief Get the directory for this media tag
   * @return the directory
   */
  const std::string& Directory() const { return m_strDirectory; }

  /*!
   * @brief Get the priority for this media tag
   * @return the priority
   */
  int Priority() const { return m_iPriority; }

  /*!
   * @brief Return the icon path as given by the client.
   * @return The path.
   */
  const std::string& ClientIconPath() const { return m_iconPath.GetClientImage(); }

  /*!
   * @brief Return the thumbnail path as given by the client.
   * @return The path.
   */
  const std::string& ClientThumbnailPath() const { return m_thumbnailPath.GetClientImage(); }

  /*!
   * @brief Return the fanart path as given by the client.
   * @return The path.
   */
  const std::string& ClientFanartPath() const { return m_fanartPath.GetClientImage(); }

  /*!
   * @brief Return the icon path used by Kodi.
   * @return The path.
   */
  const std::string& IconPath() const { return m_iconPath.GetLocalImage(); }

  /*!
   * @brief Return the thumnail path used by Kodi.
   * @return The path.
   */
  const std::string& ThumbnailPath() const { return m_thumbnailPath.GetLocalImage(); }

  /*!
   * @brief Return the fanart path used by Kodi.
   * @return The path.
   */
  const std::string& FanartPath() const { return m_fanartPath.GetLocalImage(); }

  /*!
   * @brief Retrieve the media tag Episode Name
   * @note Returns an empty string if no Episode Name was provided by the PVR client
   */
  const std::string& EpisodeName() const { return m_strShowTitle; }

  // /*!
  //  * @brief check whether this media tag is currently in progress
  //  * @return true if the media tag is in progress, false otherwise
  //  */
  // bool IsInProgress() const;

  // /*!
  //  * @brief return the timer for an in-progress media tag, if any
  //  * @return the timer if the media tag is in progress, nullptr otherwise
  //  */
  // std::shared_ptr<CPVRTimerInfoTag> GetMediaTagTimer() const;

  /*!
   * @brief set the genre for this media tag.
   * @param iGenreType The genre type ID. If set to EPG_GENRE_USE_STRING, set genre to the value provided with strGenre. Otherwise, compile the genre string from the values given by iGenreType and iGenreSubType
   * @param iGenreSubType The genre subtype ID
   * @param strGenre The genre
   */
  void SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre);

  /*!
   * @brief Get the genre type ID of this media tag.
   * @return The genre type ID.
   */
  int GenreType() const { return m_iGenreType; }

  /*!
   * @brief Get the genre subtype ID of this media tag.
   * @return The genre subtype ID.
   */
  int GenreSubType() const { return m_iGenreSubType; }

  /*!
   * @brief Get the genre as human readable string.
   * @return The genre.
   */
  const std::vector<std::string>& Genre() const { return m_genre; }

  /*!
   * @brief Get the genre(s) of this media tag as formatted string.
   * @return The genres label.
   */
  const std::string GetGenresLabel() const;

  /*!
   * @brief Get the first air date of this media tag.
   * @return The first air date.
   */
  CDateTime FirstAired() const;

  /*!
   * @brief Get the premiere year of this media tag.
   * @return The premiere year
   */
  int GetYear() const override;

  /*!
   * @brief Set the premiere year of this media tag.
   * @param year The premiere year
   */
  void SetYear(int year) override;

  /*!
   * @brief Check if the premiere year of this media tag is valid
   * @return True if the media tag has as valid premiere date, false otherwise
   */
  bool HasYear() const override;

  /*!
   * @brief Check whether this media tag will be flagged as new.
   * @return True if this media tag will be flagged as new, false otherwise
   */
  bool IsNew() const;

  /*!
   * @brief Check whether this media tag will be flagged as a premiere.
   * @return True if this media tag will be flagged as a premiere, false otherwise
   */
  bool IsPremiere() const;

  /*!
   * @brief Check whether this media tag will be flagged as a finale.
   * @return True if this media tag will be flagged as a finale, false otherwise
   */
  bool IsFinale() const;

  /*!
   * @brief Check whether this media tag will be flagged as live.
   * @return True if this media tag will be flagged as live, false otherwise
   */
  bool IsLive() const;

  /*!
   * @brief Return the flags (PVR_MEDIA_TAG_FLAG_*) of this media tag as a bitfield.
   * @return the flags.
   */
  unsigned int Flags() const { return m_iFlags; }

  /*!
   * @brief Return the size of this media tag in bytes.
   * @return the size in bytes.
   */
  int64_t GetSizeInBytes() const;

  /*!
   * @brief Mark a media tag as dirty/clean.
   * @param bDirty true to mark as dirty, false to mark as clean.
   */
  void SetDirty(bool bDirty) { m_bDirty = bDirty; }

  /*!
   * @brief Return whether the media tag is marked dirty.
   * @return true if dirty, false otherwise.
   */
  bool IsDirty() const { return m_bDirty; }

  /*!
   * @brief Get the uid of the provider on the client which this media tag is from
   * @return the client uid of the provider or PVR_PROVIDER_INVALID_UID
   */
  int ClientProviderUniqueId() const;

  /*!
   * @brief Get the client provider name for this media tag
   * @return m_strProviderName The name for this media tag provider
   */
  std::string ProviderName() const;

  /*!
   * @brief Get the default provider of this media tag. The default
   *        provider represents the PVR add-on itself.
   * @return The default provider of this media tag
   */
  std::shared_ptr<CPVRProvider> GetDefaultProvider() const;

  /*!
   * @brief Whether or not this media tag has a provider set by the client.
   * @return True if a provider was set by the client, false otherwise.
   */
  bool HasClientProvider() const;

  /*!
   * @brief Get the provider of this media tag. This may be the default provider or a
   *        custom provider set by the client. If @ref "HasClientProvider()" returns true
   *        the provider will be custom from the client, otherwise the default provider.
   * @return The provider of this media tag
   */
  std::shared_ptr<CPVRProvider> GetProvider() const;

  /*!
   * @brief Get the parental rating of this media tag.
   * @return The parental rating.
   */
  unsigned int GetParentalRating() const;

  /*!
   * @brief Get the parental rating code of this media tag.
   * @return The parental rating code.
   */
  const std::string& GetParentalRatingCode() const;

  /*!
   * @brief Get the parental rating icon path of this media tag.
   * @return The parental rating icon path.
   */
  const std::string& GetParentalRatingIcon() const;

  /*!
   * @brief Get the parental rating source of this media tag.
   * @return The parental rating source.
   */
  const std::string& GetParentalRatingSource() const;

  /*!
   * @brief Get the episode part number of this media tag.
   * @return The episode part number.
   */
  int EpisodePart() const;

private:
  CPVRMediaTag(const CPVRMediaTag& tag) = delete;
  CPVRMediaTag& operator=(const CPVRMediaTag& other) = delete;

  void UpdatePath();

  int m_iClientId; /*!< ID of the backend */
  std::string m_strMediaTagId; /*!< unique ID of the media tag on the client */
  int m_iPriority; /*!< priority of this media tag */
  std::string m_strDirectory; /*!< directory of this media tag on the client */
  unsigned int m_iMediaTagId; /*!< id that won't change while xbmc is running */

  CPVRCachedImage m_iconPath; /*!< icon path */
  CPVRCachedImage m_thumbnailPath; /*!< thumbnail path */
  CPVRCachedImage m_fanartPath; /*!< fanart path */
  CDateTime m_mediaTagTime; /*!< start time of the media tag */
  bool m_bGotMetaData;
  bool m_bRadio; /*!< radio or tv media tag */
  PVR_MEDIA_TAG_TYPE m_mediaType; /*!< typer of media stored in tag */
  int m_iGenreType = 0; /*!< genre type */
  int m_iGenreSubType = 0; /*!< genre subtype */
  mutable XbmcThreads::EndTime<> m_resumePointRefetchTimeout;
  unsigned int m_iFlags = 0; /*!< the flags applicable to this media tag */
  int64_t m_sizeInBytes = 0; /*!< the size of the media tag in bytes */
  bool m_bDirty = false;
  std::string m_strProviderName; /*!< name of the provider this media tag is from */
  int m_iClientProviderUniqueId =
      PVR_PROVIDER_INVALID_UID; /*!< provider uid associated with this media tag on the client */
  unsigned int m_parentalRating{0}; /*!< parental rating */
  std::string m_parentalRatingCode; /*!< Parental rating code */
  std::string m_parentalRatingIcon; /*!< parental rating icon path */
  std::string m_parentalRatingSource; /*!< parental rating source */
  int m_episodePartNumber{PVR_MEDIA_TAG_INVALID_SERIES_EPISODE}; /*!< episode part number */

  mutable CCriticalSection m_critSection;
};
} // namespace PVR
