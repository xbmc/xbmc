/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
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

struct PVR_EDL_ENTRY;
struct PVR_RECORDING;

namespace PVR
{
class CPVRChannel;
class CPVRClient;
class CPVRProvider;
class CPVRTimerInfoTag;

/*!
   * @brief Representation of a CPVRRecording unique ID.
   */
class CPVRRecordingUid final
{
public:
  int m_iClientId; /*!< ID of the backend */
  std::string m_strRecordingId; /*!< unique ID of the recording on the client */

  CPVRRecordingUid(int iClientId, const std::string& strRecordingId);

  bool operator>(const CPVRRecordingUid& right) const;
  bool operator<(const CPVRRecordingUid& right) const;
  bool operator==(const CPVRRecordingUid& right) const;
  bool operator!=(const CPVRRecordingUid& right) const;
};

class CPVRRecording final : public CVideoInfoTag
{
public:
  static const std::string IMAGE_OWNER_PATTERN;

  CPVRRecording();
  CPVRRecording(const PVR_RECORDING& recording, unsigned int iClientId);

  bool operator==(const CPVRRecording& right) const;
  bool operator!=(const CPVRRecording& right) const;

  /*!
   * @brief Copy over data to the given PVR_RECORDING instance.
   * @param recording The recording instance to fill.
   */
  void FillAddonData(PVR_RECORDING& recording) const;

  void Serialize(CVariant& value) const override;

  // ISortable implementation
  void ToSortable(SortItem& sortable, Field field) const override;

  /*!
   * @brief Reset this tag to it's initial state.
   */
  void Reset();

  /*!
   * @brief Delete this recording on the client (if supported).
   * @return True if it was deleted successfully, false otherwise.
   */
  bool Delete();

  /*!
   * @brief Undelete this recording on the client (if supported).
   * @return True if it was undeleted successfully, false otherwise.
   */
  bool Undelete();

  /*!
   * @brief Rename this recording on the client (if supported).
   * @param strNewName The new name.
   * @return True if it was renamed successfully, false otherwise.
   */
  bool Rename(const std::string& strNewName);

  /*!
   * @brief Set this recording's play count. The value will be transferred to the backend if it supports server-side play counts.
   * @param count play count.
   * @return True if play count was set successfully, false otherwise.
   */
  bool SetPlayCount(int count) override;

  /*!
   * @brief Increment this recording's play count. The value will be transferred to the backend if it supports server-side play counts.
   * @return True if play count was increased successfully, false otherwise.
   */
  bool IncrementPlayCount() override;

  /*!
   * @brief Set this recording's play count without transferring the value to the backend, even if it supports server-side play counts.
   * @param count play count.
   * @return True if play count was set successfully, false otherwise.
   */
  bool SetLocalPlayCount(int count) { return CVideoInfoTag::SetPlayCount(count); }

  /*!
   * @brief Get this recording's local play count. The value will not be obtained from the backend, even if it supports server-side play counts.
   * @return the play count.
   */
  int GetLocalPlayCount() const { return CVideoInfoTag::GetPlayCount(); }

  /*!
   * @brief Set this recording's resume point. The value will be transferred to the backend if it supports server-side resume points.
   * @param resumePoint resume point.
   * @return True if resume point was set successfully, false otherwise.
   */
  bool SetResumePoint(const CBookmark& resumePoint) override;

  /*!
   * @brief Set this recording's resume point. The value will be transferred to the backend if it supports server-side resume points.
   * @param timeInSeconds the time of the resume point
   * @param totalTimeInSeconds the total time of the video
   * @param playerState the player state
   * @return True if resume point was set successfully, false otherwise.
   */
  bool SetResumePoint(double timeInSeconds,
                      double totalTimeInSeconds,
                      const std::string& playerState = "") override;

  /*!
   * @brief Get this recording's resume point. The value will be obtained from the backend if it supports server-side resume points.
   * @return the resume point.
   */
  CBookmark GetResumePoint() const override;

  /*!
   * @brief Update this recording's size. The value will be obtained from the backend if it supports server-side size retrieval.
   * @return true if the the updated value is different, false otherwise.
   */
  bool UpdateRecordingSize();

  /*!
   * @brief Get this recording's local resume point. The value will not be obtained from the backend even if it supports server-side resume points.
   * @return the resume point.
   */
  CBookmark GetLocalResumePoint() const { return CVideoInfoTag::GetResumePoint(); }

  /*!
   * @brief Retrieve the edit decision list (EDL) of a recording on the backend.
   * @return The edit decision list (empty on error)
   */
  std::vector<PVR_EDL_ENTRY> GetEdl() const;

  /*!
   * @brief Get the resume point and play count from the database if the
   * client doesn't handle it itself.
   * @param db The database to read the data from.
   * @param client The client this recording belongs to.
   */
  void UpdateMetadata(CVideoDatabase& db, const CPVRClient& client);

  /*!
   * @brief Update this tag with the contents of the given tag.
   * @param tag The new tag info.
   * @param client The client this recording belongs to.
   */
  void Update(const CPVRRecording& tag, const CPVRClient& client);

  /*!
   * @brief Retrieve the recording start as UTC time
   * @return the recording start time
   */
  const CDateTime& RecordingTimeAsUTC() const { return m_recordingTime; }

  /*!
   * @brief Retrieve the recording start as local time
   * @return the recording start time
   */
  const CDateTime& RecordingTimeAsLocalTime() const;

  /*!
   * @brief Retrieve the recording end as UTC time
   * @return the recording end time
   */
  CDateTime EndTimeAsUTC() const;

  /*!
   * @brief Retrieve the recording end as local time
   * @return the recording end time
   */
  CDateTime EndTimeAsLocalTime() const;

  /*!
   * @brief Check whether this recording has an expiration time
   * @return True if the recording has an expiration time, false otherwise
   */
  bool HasExpirationTime() const { return m_iLifetime > 0; }

  /*!
   * @brief Retrieve the recording expiration time as local time
   * @return the recording expiration time
   */
  CDateTime ExpirationTimeAsLocalTime() const;

  /*!
   * @brief Check whether this recording will immediately expire if the given lifetime value would be set
   * @param iLifetime The lifetime value to check
   * @return True if the recording would immediately expire, false otherwiese
   */
  bool WillBeExpiredWithNewLifetime(int iLifetime) const;

  /*!
   * @brief Retrieve the recording title from the URL path
   * @param url the URL for the recording
   * @return Title of the recording
   */
  static std::string GetTitleFromURL(const std::string& url);

  /*!
   * @brief If deleted but can be undeleted it is true
   */
  bool IsDeleted() const { return m_bIsDeleted; }

  /*!
   * @brief Check whether this is a tv or radio recording
   * @return true if this is a radio recording, false if this is a tv recording
   */
  bool IsRadio() const { return m_bRadio; }

  /*!
   * @return Broadcast id of the EPG event associated with this recording or EPG_TAG_INVALID_UID
   */
  unsigned int BroadcastUid() const { return m_iEpgEventId; }

  /*!
   * @return Get the channel on which this recording is/was running
   * @note Only works if the recording has a channel uid provided by the add-on
   */
  std::shared_ptr<CPVRChannel> Channel() const;

  /*!
   * @brief Get the uid of the channel on which this recording is/was running
   * @return the uid of the channel or PVR_CHANNEL_INVALID_UID
   */
  int ChannelUid() const;

  /*!
   * @brief the identifier of the client that serves this recording
   * @return the client identifier
   */
  int ClientID() const;

  /*!
   * @brief Get the recording ID as upplied by the client
   * @return the recording identifier
   */
  std::string ClientRecordingID() const { return m_strRecordingId; }

  /*!
   * @brief Get the recording ID as upplied by the client
   * @return the recording identifier
   */
  unsigned int RecordingID() const { return m_iRecordingId; }

  /*!
   * @brief Set the recording ID
   * @param recordingId The new identifier
   */
  void SetRecordingID(unsigned int recordingId) { m_iRecordingId = recordingId; }

  /*!
   * @brief Get the directory for this recording
   * @return the directory
   */
  std::string Directory() const { return m_strDirectory; }

  /*!
   * @brief Get the priority for this recording
   * @return the priority
   */
  int Priority() const { return m_iPriority; }

  /*!
   * @brief Get the lifetime for this recording
   * @return the lifetime
   */
  int LifeTime() const { return m_iLifetime; }

  /*!
   * @brief Set the lifetime for this recording
   * @param lifeTime The lifetime
   */
  void SetLifeTime(int lifeTime) { m_iLifetime = lifeTime; }

  /*!
   * @brief Get the channel name for this recording
   * @return the channel name
   */
  std::string ChannelName() const { return m_strChannelName; }

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
   * @brief Retrieve the recording Episode Name
   * @note Returns an empty string if no Episode Name was provided by the PVR client
   */
  std::string EpisodeName() const { return m_strShowTitle; }

  /*!
   * @brief check whether this recording is currently in progress
   * @return true if the recording is in progress, false otherwise
   */
  bool IsInProgress() const;

  /*!
   * @brief return the timer for an in-progress recording, if any
   * @return the timer if the recording is in progress, nullptr otherwise
   */
  std::shared_ptr<CPVRTimerInfoTag> GetRecordingTimer() const;

  /*!
   * @brief set the genre for this recording.
   * @param iGenreType The genre type ID. If set to EPG_GENRE_USE_STRING, set genre to the value provided with strGenre. Otherwise, compile the genre string from the values given by iGenreType and iGenreSubType
   * @param iGenreSubType The genre subtype ID
   * @param strGenre The genre
   */
  void SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre);

  /*!
   * @brief Get the genre type ID of this recording.
   * @return The genre type ID.
   */
  int GenreType() const { return m_iGenreType; }

  /*!
   * @brief Get the genre subtype ID of this recording.
   * @return The genre subtype ID.
   */
  int GenreSubType() const { return m_iGenreSubType; }

  /*!
   * @brief Get the genre as human readable string.
   * @return The genre.
   */
  const std::vector<std::string> Genre() const { return m_genre; }

  /*!
   * @brief Get the genre(s) of this recording as formatted string.
   * @return The genres label.
   */
  const std::string GetGenresLabel() const;

  /*!
   * @brief Get the first air date of this recording.
   * @return The first air date.
   */
  CDateTime FirstAired() const;

  /*!
   * @brief Get the premiere year of this recording.
   * @return The premiere year
   */
  int GetYear() const override;

  /*!
   * @brief Set the premiere year of this recording.
   * @param year The premiere year
   */
  void SetYear(int year) override;

  /*!
   * @brief Check if the premiere year of this recording is valid
   * @return True if the recording has as valid premiere date, false otherwise
   */
  bool HasYear() const override;

  /*!
   * @brief Check whether this recording will be flagged as new.
   * @return True if this recording will be flagged as new, false otherwise
   */
  bool IsNew() const;

  /*!
   * @brief Check whether this recording will be flagged as a premiere.
   * @return True if this recording will be flagged as a premiere, false otherwise
   */
  bool IsPremiere() const;

  /*!
   * @brief Check whether this recording will be flagged as a finale.
   * @return True if this recording will be flagged as a finale, false otherwise
   */
  bool IsFinale() const;

  /*!
   * @brief Check whether this recording will be flagged as live.
   * @return True if this recording will be flagged as live, false otherwise
   */
  bool IsLive() const;

  /*!
   * @brief Return the flags (PVR_RECORDING_FLAG_*) of this recording as a bitfield.
   * @return the flags.
   */
  unsigned int Flags() const { return m_iFlags; }

  /*!
   * @brief Return the size of this recording in bytes.
   * @return the size in bytes.
   */
  int64_t GetSizeInBytes() const;

  /*!
   * @brief Mark a recording as dirty/clean.
   * @param bDirty true to mark as dirty, false to mark as clean.
   */
  void SetDirty(bool bDirty) { m_bDirty = bDirty; }

  /*!
   * @brief Return whether the recording is marked dirty.
   * @return true if dirty, false otherwise.
   */
  bool IsDirty() const { return m_bDirty; }

  /*!
   * @brief Get the uid of the provider on the client which this recording is from
   * @return the client uid of the provider or PVR_PROVIDER_INVALID_UID
   */
  int ClientProviderUniqueId() const;

  /*!
   * @brief Get the client provider name for this recording
   * @return m_strProviderName The name for this recording provider
   */
  std::string ProviderName() const;

  /*!
   * @brief Get the default provider of this recording. The default
   *        provider represents the PVR add-on itself.
   * @return The default provider of this recording
   */
  std::shared_ptr<CPVRProvider> GetDefaultProvider() const;

  /*!
   * @brief Whether or not this recording has a provider set by the client.
   * @return True if a provider was set by the client, false otherwise.
   */
  bool HasClientProvider() const;

  /*!
   * @brief Get the provider of this recording. This may be the default provider or a
   *        custom provider set by the client. If @ref "HasClientProvider()" returns true
   *        the provider will be custom from the client, otherwise the default provider.
   * @return The provider of this recording
   */
  std::shared_ptr<CPVRProvider> GetProvider() const;

private:
  CPVRRecording(const CPVRRecording& tag) = delete;
  CPVRRecording& operator=(const CPVRRecording& other) = delete;

  void UpdatePath();

  int m_iClientId; /*!< ID of the backend */
  std::string m_strRecordingId; /*!< unique ID of the recording on the client */
  std::string m_strChannelName; /*!< name of the channel this was recorded from */
  int m_iPriority; /*!< priority of this recording */
  int m_iLifetime; /*!< lifetime of this recording */
  std::string m_strDirectory; /*!< directory of this recording on the client */
  unsigned int m_iRecordingId; /*!< id that won't change while xbmc is running */

  CPVRCachedImage m_iconPath; /*!< icon path */
  CPVRCachedImage m_thumbnailPath; /*!< thumbnail path */
  CPVRCachedImage m_fanartPath; /*!< fanart path */
  CDateTime m_recordingTime; /*!< start time of the recording */
  bool m_bGotMetaData;
  bool m_bIsDeleted; /*!< set if entry is a deleted recording which can be undelete */
  mutable bool m_bInProgress; /*!< set if recording might be in progress */
  unsigned int m_iEpgEventId; /*!< epg broadcast id associated with this recording */
  int m_iChannelUid; /*!< channel uid associated with this recording */
  bool m_bRadio; /*!< radio or tv recording */
  int m_iGenreType = 0; /*!< genre type */
  int m_iGenreSubType = 0; /*!< genre subtype */
  mutable XbmcThreads::EndTime<> m_resumePointRefetchTimeout;
  unsigned int m_iFlags = 0; /*!< the flags applicable to this recording */
  mutable XbmcThreads::EndTime<> m_recordingSizeRefetchTimeout;
  int64_t m_sizeInBytes = 0; /*!< the size of the recording in bytes */
  bool m_bDirty = false;
  std::string m_strProviderName; /*!< name of the provider this recording is from */
  int m_iClientProviderUniqueId =
      PVR_PROVIDER_INVALID_UID; /*!< provider uid associated with this recording on the client */

  mutable CCriticalSection m_critSection;
};
} // namespace PVR
