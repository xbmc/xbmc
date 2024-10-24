/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 5 - PVR recordings
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording class PVRRecording
/// @ingroup cpp_kodi_addon_pvr_Defs_Recording
/// @brief **Data structure with available recordings data**\n
/// With this, recordings related data are transferred between addon and Kodi
/// and can also be used by the addon itself.
///
/// The related values here are automatically initiated to defaults and need
/// only be set if supported and used.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Recording_PVRRecording_Help
///
///@{
class PVRRecording : public DynamicCStructHdl<PVRRecording, PVR_RECORDING>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRRecording()
  {
    m_cStructure->iSeriesNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodePartNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    m_cStructure->iDuration = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iPriority = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iLifetime = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreType = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreSubType = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iPlayCount = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iLastPlayedPosition = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iChannelUid = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;
    m_cStructure->sizeInBytes = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iClientProviderUid = PVR_PROVIDER_INVALID_UID;
  }
  PVRRecording(const PVRRecording& recording) : DynamicCStructHdl(recording) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Recording id** | `std::string` | @ref PVRRecording::SetRecordingId "SetRecordingId" | @ref PVRRecording::GetRecordingId "GetRecordingId" | *required to set*
  /// | **Title** | `std::string` | @ref PVRRecording::SetTitle "SetTitle" | @ref PVRRecording::GetTitle "GetTitle" | *required to set*
  /// | **Title extra info** | `std::string` | @ref PVRRecording::SetTitleExtraInfo "SetTitleExtraInfo" | @ref PVRRecording::GetTitleExtraInfo "GetTitleExtraInfo" | *optional*
  /// | **Episode name** | `std::string` | @ref PVRRecording::SetEpisodeName "SetEpisodeName" | @ref PVRRecording::GetEpisodeName "GetEpisodeName" | *optional*
  /// | **Series number** | `int` | @ref PVRRecording::SetSeriesNumber "SetSeriesNumber" | @ref PVRRecording::GetSeriesNumber "GetSeriesNumber" | *optional*
  /// | **Episode number** | `int` | @ref PVRRecording::SetEpisodeNumber "SetEpisodeNumber" | @ref PVRRecording::GetEpisodeNumber "GetEpisodeNumber" | *optional*
  /// | **Episode part number** | `int` | @ref PVRRecording::SetEpisodePartNumber "SetEpisodePartNumber" | @ref PVRRecording::GetEpisodePartNumber "GetEpisodePartNumber" | *optional*
  /// | **Year** | `int` | @ref PVRRecording::SetYear "SetYear" | @ref PVRRecording::GetYear "GetYear" | *optional*
  /// | **Directory** | `std::string` | @ref PVRRecording::SetDirectory "SetDirectory" | @ref PVRRecording::GetDirectory "GetDirectory" | *optional*
  /// | **Plot outline** | `std::string` | @ref PVRRecording::SetPlotOutline "SetPlotOutline" | @ref PVRRecording::GetPlotOutline "GetPlotOutline" | *optional*
  /// | **Plot** | `std::string` | @ref PVRRecording::SetPlot "SetPlot" | @ref PVRRecording::GetPlot "GetPlot" | *optional*
  /// | **Genre description** | `std::string` | @ref PVRRecording::SetGenreDescription "SetGenreDescription" | @ref PVRRecording::GetGenreDescription "GetGenreDescription" | *optional*
  /// | **Channel name** | `std::string` | @ref PVRRecording::SetChannelName "SetChannelName" | @ref PVRRecording::GetChannelName "GetChannelName" | *optional*
  /// | **Icon path** | `std::string` | @ref PVRRecording::SetIconPath "SetIconPath" | @ref PVRRecording::GetIconPath "GetIconPath" | *optional*
  /// | **Thumbnail path** | `std::string` | @ref PVRRecording::SetThumbnailPath "SetThumbnailPath" | @ref PVRRecording::GetThumbnailPath "GetThumbnailPath" | *optional*
  /// | **Fanart path** | `std::string` | @ref PVRRecording::SetFanartPath "SetFanartPath" | @ref PVRRecording::GetFanartPath "GetFanartPath" | *optional*
  /// | **Recording time** | `time_t` | @ref PVRRecording::SetRecordingTime "SetRecordingTime" | @ref PVRRecording::GetRecordingTime "GetRecordingTime" | *optional*
  /// | **Duration** | `int` | @ref PVRRecording::SetDuration "SetDuration" | @ref PVRRecording::GetDuration "GetDuration" | *optional*
  /// | **Priority** | `int` | @ref PVRRecording::SetPriority "SetPriority" | @ref PVRRecording::GetPriority "GetPriority" | *optional*
  /// | **Lifetime** | `int` | @ref PVRRecording::SetLifetime "SetLifetime" | @ref PVRRecording::GetLifetime "GetLifetime" | *optional*
  /// | **Genre type** | `int` | @ref PVRRecording::SetGenreType "SetGenreType" | @ref PVRRecording::GetGenreType "GetGenreType" | *optional*
  /// | **Genre sub type** | `int` | @ref PVRRecording::SetGenreSubType "SetGenreSubType" | @ref PVRRecording::GetGenreSubType "GetGenreSubType" | *optional*
  /// | **Play count** | `int` | @ref PVRRecording::SetPlayCount "SetPlayCount" | @ref PVRRecording::GetPlayCount "GetPlayCount" | *optional*
  /// | **Last played position** | `int` | @ref PVRRecording::SetLastPlayedPosition "SetLastPlayedPosition" | @ref PVRRecording::GetLastPlayedPosition "GetLastPlayedPosition" | *optional*
  /// | **Is deleted** | `bool` | @ref PVRRecording::SetIsDeleted "SetIsDeleted" | @ref PVRRecording::GetIsDeleted "GetIsDeleted" | *optional*
  /// | **EPG event id** | `unsigned int` | @ref PVRRecording::SetEPGEventId "SetEPGEventId" | @ref PVRRecording::GetEPGEventId "GetEPGEventId" | *optional*
  /// | **Channel unique id** | `int` | @ref PVRRecording::SetChannelUid "SetChannelUid" | @ref PVRRecording::GetChannelUid "GetChannelUid" | *optional*
  /// | **Channel type** | @ref PVR_RECORDING_CHANNEL_TYPE | @ref PVRRecording::SetChannelType "SetChannelType" | @ref PVRRecording::GetChannelType "GetChannelType" | *optional*
  /// | **First aired** | `std::string` | @ref PVRRecording::SetFirstAired "SetFirstAired" | @ref PVRRecording::GetFirstAired "GetFirstAired" | *optional*
  /// | **Flags** | `std::string` | @ref PVRRecording::SetFlags "SetFlags" | @ref PVRRecording::GetFlags "GetFlags" | *optional*
  /// | **Size in bytes** | `std::string` | @ref PVRRecording::SetSizeInBytes "SetSizeInBytes" | @ref PVRRecording::GetSizeInBytes "GetSizeInBytes" | *optional*
  /// | **Client provider unique identifier** | `int` | @ref PVRChannel::SetClientProviderUid "SetClientProviderUid" | @ref PVRTimer::GetClientProviderUid "GetClientProviderUid" | *optional*
  /// | **Provider name** | `std::string` | @ref PVRChannel::SetProviderName "SetProviderlName" | @ref PVRChannel::GetProviderName "GetProviderName" | *optional*
  /// | **Parental rating age** | `unsigned int` | @ref PVRRecording::SetParentalRating "SetParentalRating" | @ref PVRRecording::GetParentalRating "GetParentalRating" | *optional*
  /// | **Parental rating code** | `std::string` | @ref PVRRecording::SetParentalRatingCode "SetParentalRatingCode" | @ref PVRRecording::GetParentalRatingCode "GetParentalRatingCode" | *optional*
  /// | **Parental rating icon** | `std::string` | @ref PVRRecording::SetParentalRatingIcon "SetParentalRatingIcon" | @ref PVRRecording::GetParentalRatingIcon "GetParentalRatingIcon" | *optional*
  /// | **Parental rating source** | `std::string` | @ref PVRRecording::SetParentalRatingSource "SetParentalRatingSource" | @ref PVRRecording::GetParentalRatingSource "GetParentalRatingSource" | *optional*

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///@{

  /// @brief **required**\n
  /// Unique identifier of the recording on the client.
  void SetRecordingId(const std::string& recordingId)
  {
    ReallocAndCopyString(&m_cStructure->strRecordingId, recordingId.c_str());
  }

  /// @brief To get with @ref SetRecordingId changed values.
  std::string GetRecordingId() const
  {
    return m_cStructure->strRecordingId ? m_cStructure->strRecordingId : "";
  }

  /// @brief **required**\n
  /// The title of this recording.
  void SetTitle(const std::string& title)
  {
    ReallocAndCopyString(&m_cStructure->strTitle, title.c_str());
  }

  /// @brief To get with @ref SetTitle changed values.
  std::string GetTitle() const { return m_cStructure->strTitle ? m_cStructure->strTitle : ""; }

  /// @brief **optional**\n
  /// The title extra information of this recording.
  void SetTitleExtraInfo(const std::string& titleExtraInfo)
  {
    ReallocAndCopyString(&m_cStructure->strTitleExtraInfo, titleExtraInfo.c_str());
  }

  /// @brief To get with @ref SetTitleExtraInfo changed values.
  std::string GetTitleExtraInfo() const
  {
    return m_cStructure->strTitleExtraInfo ? m_cStructure->strTitleExtraInfo : "";
  }

  /// @brief **optional**\n
  /// Episode name (also known as subtitle).
  void SetEpisodeName(const std::string& episodeName)
  {
    ReallocAndCopyString(&m_cStructure->strEpisodeName, episodeName.c_str());
  }

  /// @brief To get with @ref SetEpisodeName changed values.
  std::string GetEpisodeName() const
  {
    return m_cStructure->strEpisodeName ? m_cStructure->strEpisodeName : "";
  }

  /// @brief **optional**\n
  /// Series number (usually called season).
  ///
  /// Set to "0" for specials/pilot. For 'invalid' see @ref SetEpisodeNumber or set to -1.
  void SetSeriesNumber(int seriesNumber) { m_cStructure->iSeriesNumber = seriesNumber; }

  /// @brief To get with @ref SetSeriesNumber changed values.
  int GetSeriesNumber() const { return m_cStructure->iSeriesNumber; }

  /// @brief **optional**\n
  /// Eepisode number within the "iSeriesNumber" season.
  ///
  /// For 'invalid' set to -1 or seriesNumber=episodeNumber=0 to show both are invalid.
  void SetEpisodeNumber(int episodeNumber) { m_cStructure->iEpisodeNumber = episodeNumber; }

  /// @brief To get with @ref SetEpisodeNumber changed values.
  int GetEpisodeNumber() const { return m_cStructure->iEpisodeNumber; }

  /// @brief **optional**\n
  /// Episode part number.
  void SetEpisodePartNumber(int episodePartNumber)
  {
    m_cStructure->iEpisodePartNumber = episodePartNumber;
  }

  /// @brief To get with @ref SetEpisodePartNumber changed values.
  int GetEpisodePartNumber() const { return m_cStructure->iEpisodePartNumber; }

  /// @brief **optional**\n
  /// Year of first release (use to identify a specific movie re-make) / first
  /// airing for TV shows.
  ///
  /// Set to '0' for invalid.
  void SetYear(int year) { m_cStructure->iYear = year; }

  /// @brief To get with @ref SetYear changed values.
  int GetYear() const { return m_cStructure->iYear; }

  /// @brief **optional**\n
  ///
  /// Directory of this recording on the client.
  void SetDirectory(const std::string& directory)
  {
    ReallocAndCopyString(&m_cStructure->strDirectory, directory.c_str());
  }

  /// @brief To get with @ref SetDirectory changed values.
  std::string GetDirectory() const
  {
    return m_cStructure->strDirectory ? m_cStructure->strDirectory : "";
  }

  /// @brief **optional**\n
  /// Plot outline name.
  void SetPlotOutline(const std::string& plotOutline)
  {
    ReallocAndCopyString(&m_cStructure->strPlotOutline, plotOutline.c_str());
  }

  /// @brief To get with @ref SetPlotOutline changed values.
  std::string GetPlotOutline() const
  {
    return m_cStructure->strPlotOutline ? m_cStructure->strPlotOutline : "";
  }

  /// @brief **optional**\n
  /// Plot name.
  void SetPlot(const std::string& plot)
  {
    ReallocAndCopyString(&m_cStructure->strPlot, plot.c_str());
  }

  /// @brief To get with @ref SetPlot changed values.
  std::string GetPlot() const { return m_cStructure->strPlot ? m_cStructure->strPlot : ""; }

  /// @brief **optional**\n
  /// Channel name.
  void SetChannelName(const std::string& channelName)
  {
    ReallocAndCopyString(&m_cStructure->strChannelName, channelName.c_str());
  }

  /// @brief To get with @ref SetChannelName changed values.
  std::string GetChannelName() const
  {
    return m_cStructure->strChannelName ? m_cStructure->strChannelName : "";
  }

  /// @brief **optional**\n
  /// Channel logo (icon) path.
  void SetIconPath(const std::string& iconPath)
  {
    ReallocAndCopyString(&m_cStructure->strIconPath, iconPath.c_str());
  }

  /// @brief To get with @ref SetIconPath changed values.
  std::string GetIconPath() const
  {
    return m_cStructure->strIconPath ? m_cStructure->strIconPath : "";
  }

  /// @brief **optional**\n
  /// Thumbnail path.
  void SetThumbnailPath(const std::string& thumbnailPath)
  {
    ReallocAndCopyString(&m_cStructure->strThumbnailPath, thumbnailPath.c_str());
  }

  /// @brief To get with @ref SetThumbnailPath changed values.
  std::string GetThumbnailPath() const
  {
    return m_cStructure->strThumbnailPath ? m_cStructure->strThumbnailPath : "";
  }

  /// @brief **optional**\n
  /// Fanart path.
  void SetFanartPath(const std::string& fanartPath)
  {
    ReallocAndCopyString(&m_cStructure->strFanartPath, fanartPath.c_str());
  }

  /// @brief To get with @ref SetFanartPath changed values.
  std::string GetFanartPath() const
  {
    return m_cStructure->strFanartPath ? m_cStructure->strFanartPath : "";
  }

  /// @brief **optional**\n
  /// Start time of the recording.
  void SetRecordingTime(time_t recordingTime) { m_cStructure->recordingTime = recordingTime; }

  /// @brief To get with @ref SetRecordingTime changed values.
  time_t GetRecordingTime() const { return m_cStructure->recordingTime; }

  /// @brief **optional**\n
  /// Duration of the recording in seconds.
  void SetDuration(int duration) { m_cStructure->iDuration = duration; }

  /// @brief To get with @ref SetDuration changed values.
  int GetDuration() const { return m_cStructure->iDuration; }

  /// @brief **optional**\n
  /// Priority of this recording (from 0 - 100).
  void SetPriority(int priority) { m_cStructure->iPriority = priority; }

  /// @brief To get with @ref SetPriority changed values.
  int GetPriority() const { return m_cStructure->iPriority; }

  /// @brief **optional**\n
  /// Life time in days of this recording.
  void SetLifetime(int lifetime) { m_cStructure->iLifetime = lifetime; }

  /// @brief To get with @ref SetLifetime changed values.
  int GetLifetime() const { return m_cStructure->iLifetime; }

  /// @brief **optional**\n
  /// Genre type.
  ///
  /// Use @ref EPG_GENRE_USE_STRING if type becomes given by @ref SetGenreDescription.
  ///
  /// @note If confirmed that backend brings the types in [ETSI EN 300 468](https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.14.01_60/en_300468v011401p.pdf)
  /// conform values, can be @ref EPG_EVENT_CONTENTMASK ignored and to set here
  /// with backend value.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 1:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 2** (in case of other, not ETSI EN 300 468 conform genre types):
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetGenreType(EPG_GENRE_USE_STRING);
  /// tag.SetGenreDescription("My special genre name"); // Should use (if possible) kodi::GetLocalizedString(...) to have match user language.
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }

  /// @brief To get with @ref SetGenreType changed values.
  int GetGenreType() const { return m_cStructure->iGenreType; }

  /// @brief **optional**\n
  /// Genre sub type.
  ///
  /// Subtypes groups related to set by @ref SetGenreType:
  /// | Main genre type | List with available sub genre types
  /// |-----------------|-----------------------------------------
  /// | @ref EPG_EVENT_CONTENTMASK_UNDEFINED | Nothing, should be 0
  /// | @ref EPG_EVENT_CONTENTMASK_MOVIEDRAMA | @ref EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA
  /// | @ref EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS | @ref EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS
  /// | @ref EPG_EVENT_CONTENTMASK_SHOW | @ref EPG_EVENT_CONTENTSUBMASK_SHOW
  /// | @ref EPG_EVENT_CONTENTMASK_SPORTS | @ref EPG_EVENT_CONTENTSUBMASK_SPORTS
  /// | @ref EPG_EVENT_CONTENTMASK_CHILDRENYOUTH | @ref EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH
  /// | @ref EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE | @ref EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE
  /// | @ref EPG_EVENT_CONTENTMASK_ARTSCULTURE | @ref EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE
  /// | @ref EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS | @ref EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS
  /// | @ref EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE | @ref EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE
  /// | @ref EPG_EVENT_CONTENTMASK_LEISUREHOBBIES | @ref EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES
  /// | @ref EPG_EVENT_CONTENTMASK_SPECIAL | @ref EPG_EVENT_CONTENTSUBMASK_SPECIAL
  /// | @ref EPG_EVENT_CONTENTMASK_USERDEFINED | Can be defined by you
  /// | @ref EPG_GENRE_USE_STRING | **Kodi's own value**, which declares that the type with @ref SetGenreDescription is given.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE);
  /// tag.SetGenreSubType(EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_JAZZ);
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }

  /// @brief To get with @ref SetGenreSubType changed values.
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  /// @brief **optional**\n
  /// To set own genre description name.
  ///
  /// Will be used only when genreType == @ref EPG_GENRE_USE_STRING or
  /// genreSubType == @ref EPG_GENRE_USE_STRING.
  ///
  /// Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different genres.
  ///
  /// In case of other, not [ETSI EN 300 468](https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.14.01_60/en_300468v011401p.pdf)
  /// conform genre types or something special.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetGenreType(EPG_GENRE_USE_STRING);
  /// tag.SetGenreDescription("Action" + EPG_STRING_TOKEN_SEPARATOR + "Thriller");
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreDescription(const std::string& genreDescription)
  {
    ReallocAndCopyString(&m_cStructure->strGenreDescription, genreDescription.c_str());
  }

  /// @brief To get with @ref SetGenreDescription changed values.
  std::string GetGenreDescription() const
  {
    return m_cStructure->strGenreDescription ? m_cStructure->strGenreDescription : "";
  }

  /// @brief **optional**\n
  /// Play count of this recording on the client.
  void SetPlayCount(int playCount) { m_cStructure->iPlayCount = playCount; }

  /// @brief To get with @ref SetPlayCount changed values.
  int GetPlayCount() const { return m_cStructure->iPlayCount; }

  /// @brief **optional**\n
  /// Last played position of this recording on the client.
  void SetLastPlayedPosition(int lastPlayedPosition)
  {
    m_cStructure->iLastPlayedPosition = lastPlayedPosition;
  }

  /// @brief To get with @ref SetLastPlayedPosition changed values.
  int GetLastPlayedPosition() const { return m_cStructure->iLastPlayedPosition; }

  /// @brief **optional**\n
  /// Shows this recording is deleted and can be undelete.
  void SetIsDeleted(int isDeleted) { m_cStructure->bIsDeleted = isDeleted; }

  /// @brief To get with @ref SetIsDeleted changed values.
  int GetIsDeleted() const { return m_cStructure->bIsDeleted; }

  /// @brief **optional**\n
  /// EPG event id associated with this recording. Valid ids must be greater than @ref EPG_TAG_INVALID_UID.
  void SetEPGEventId(unsigned int epgEventId) { m_cStructure->iEpgEventId = epgEventId; }

  /// @brief To get with @ref SetEPGEventId changed values.
  unsigned int GetEPGEventId() const { return m_cStructure->iEpgEventId; }

  /// @brief **optional**\n
  /// Unique identifier of the channel for this recording. @ref PVR_CHANNEL_INVALID_UID
  /// denotes that channel uid is not available.
  void SetChannelUid(int channelUid) { m_cStructure->iChannelUid = channelUid; }

  /// @brief To get with @ref SetChannelUid changed values
  int GetChannelUid() const { return m_cStructure->iChannelUid; }

  /// @brief **optional**\n
  /// Channel type.
  ///
  /// Set to @ref PVR_RECORDING_CHANNEL_TYPE_UNKNOWN if the type cannot be
  /// determined.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_TV);
  /// ~~~~~~~~~~~~~
  ///
  void SetChannelType(PVR_RECORDING_CHANNEL_TYPE channelType)
  {
    m_cStructure->channelType = channelType;
  }

  /// @brief To get with @ref SetChannelType changed values
  PVR_RECORDING_CHANNEL_TYPE GetChannelType() const { return m_cStructure->channelType; }

  /// @brief **optional**\n
  /// First aired date of this recording.
  ///
  /// Used only for display purposes. Specify in W3C date format "YYYY-MM-DD".
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetFirstAired(1982-10-22);
  /// ~~~~~~~~~~~~~
  ///
  void SetFirstAired(const std::string& firstAired)
  {
    ReallocAndCopyString(&m_cStructure->strFirstAired, firstAired.c_str());
  }

  /// @brief To get with @ref SetFirstAired changed values
  std::string GetFirstAired() const
  {
    return m_cStructure->strFirstAired ? m_cStructure->strFirstAired : "";
  }

  /// @brief **optional**\n
  /// Bit field of independent flags associated with the recording.
  ///
  /// See @ref cpp_kodi_addon_pvr_Defs_Recording_PVR_RECORDING_FLAG for
  /// available bit flags.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Recording_PVR_RECORDING_FLAG
  ///
  void SetFlags(unsigned int flags) { m_cStructure->iFlags = flags; }

  /// @brief To get with @ref SetFlags changed values.
  unsigned int GetFlags() const { return m_cStructure->iFlags; }

  /// @brief **optional**\n
  /// Size of the recording in bytes.
  void SetSizeInBytes(int64_t sizeInBytes) { m_cStructure->sizeInBytes = sizeInBytes; }

  /// @brief To get with @ref SetSizeInBytes changed values.
  int64_t GetSizeInBytes() const { return m_cStructure->sizeInBytes; }
  ///@}

  /// @brief **optional**\n
  /// Unique identifier of the provider this channel belongs to.
  ///
  /// @ref PVR_PROVIDER_INVALID_UID denotes that provider uid is not available.
  void SetClientProviderUid(int iClientProviderUid)
  {
    m_cStructure->iClientProviderUid = iClientProviderUid;
  }

  /// @brief To get with @ref SetClientProviderUid changed values
  int GetClientProviderUid() const { return m_cStructure->iClientProviderUid; }

  /// @brief **optional**\n
  /// Name for the provider of this channel.
  void SetProviderName(const std::string& providerName)
  {
    ReallocAndCopyString(&m_cStructure->strProviderName, providerName.c_str());
  }

  /// @brief To get with @ref SetProviderName changed values.
  std::string GetProviderName() const
  {
    return m_cStructure->strProviderName ? m_cStructure->strProviderName : "";
  }

  /// @brief **optional**\n
  /// Age rating for the recording.
  void SetParentalRating(unsigned int iParentalRating)
  {
    m_cStructure->iParentalRating = iParentalRating;
  }

  /// @brief To get with @ref SetParentalRating changed values
  unsigned int GetParentalRating() const { return m_cStructure->iParentalRating; }

  /// @brief **optional**\n
  /// Parental rating code for this recording.
  void SetParentalRatingCode(const std::string& ratingCode)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingCode, ratingCode.c_str());
  }

  /// @brief To get with @ref SetParentalRatingCode changed values.
  std::string GetParentalRatingCode() const
  {
    return m_cStructure->strParentalRatingCode ? m_cStructure->strParentalRatingCode : "";
  }

  /// @brief **optional**\n
  /// Parental rating icon for this recording.
  void SetParentalRatingIcon(const std::string& ratingIcon)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingIcon, ratingIcon.c_str());
  }

  /// @brief To get with @ref SetParentalRatingIcon changed values.
  std::string GetParentalRatingIcon() const
  {
    return m_cStructure->strParentalRatingIcon ? m_cStructure->strParentalRatingIcon : "";
  }

  /// @brief **optional**\n
  /// Parental rating source for this recording.
  void SetParentalRatingSource(const std::string& ratingSource)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingSource, ratingSource.c_str());
  }

  /// @brief To get with @ref SetParentalRatingSource changed values.
  std::string GetParentalRatingSource() const
  {
    return m_cStructure->strParentalRatingSource ? m_cStructure->strParentalRatingSource : "";
  }

  static void AllocResources(const PVR_RECORDING* source, PVR_RECORDING* target)
  {
    target->strRecordingId = AllocAndCopyString(source->strRecordingId);
    target->strTitle = AllocAndCopyString(source->strTitle);
    target->strTitleExtraInfo = AllocAndCopyString(source->strTitleExtraInfo);
    target->strEpisodeName = AllocAndCopyString(source->strEpisodeName);
    target->strDirectory = AllocAndCopyString(source->strDirectory);
    target->strPlotOutline = AllocAndCopyString(source->strPlotOutline);
    target->strPlot = AllocAndCopyString(source->strPlot);
    target->strGenreDescription = AllocAndCopyString(source->strGenreDescription);
    target->strChannelName = AllocAndCopyString(source->strChannelName);
    target->strIconPath = AllocAndCopyString(source->strIconPath);
    target->strThumbnailPath = AllocAndCopyString(source->strThumbnailPath);
    target->strFanartPath = AllocAndCopyString(source->strFanartPath);
    target->strFirstAired = AllocAndCopyString(source->strFirstAired);
    target->strProviderName = AllocAndCopyString(source->strProviderName);
    target->strParentalRatingCode = AllocAndCopyString(source->strParentalRatingCode);
    target->strParentalRatingIcon = AllocAndCopyString(source->strParentalRatingIcon);
    target->strParentalRatingSource = AllocAndCopyString(source->strParentalRatingSource);
  }

  static void FreeResources(PVR_RECORDING* target)
  {
    FreeString(target->strRecordingId);
    FreeString(target->strTitle);
    FreeString(target->strTitleExtraInfo);
    FreeString(target->strEpisodeName);
    FreeString(target->strDirectory);
    FreeString(target->strPlotOutline);
    FreeString(target->strPlot);
    FreeString(target->strGenreDescription);
    FreeString(target->strChannelName);
    FreeString(target->strIconPath);
    FreeString(target->strThumbnailPath);
    FreeString(target->strFanartPath);
    FreeString(target->strFirstAired);
    FreeString(target->strProviderName);
    FreeString(target->strParentalRatingCode);
    FreeString(target->strParentalRatingIcon);
    FreeString(target->strParentalRatingSource);
  }

private:
  PVRRecording(const PVR_RECORDING* recording) : DynamicCStructHdl(recording) {}
  PVRRecording(PVR_RECORDING* recording) : DynamicCStructHdl(recording) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecordingsResultSet class PVRRecordingsResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
/// @brief **PVR add-on recording transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetRecordings().
///
/// @note This becomes only be used on addon call above, not usable outside on
/// addon itself.
///@{
class PVRRecordingsResultSet
{
public:
  /*! \cond PRIVATE */
  PVRRecordingsResultSet() = delete;
  PVRRecordingsResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecordingsResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRRecording& tag)
  {
    m_instance->toKodi->TransferRecordingEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
