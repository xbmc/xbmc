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
// "C++" Definitions group 7 - PVR media
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag class PVRMediaTag
/// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag
/// @brief **Data structure with available media data**\n
/// With this, media related data are transferred between addon and Kodi
/// and can also be used by the addon itself.
///
/// The related values here are automatically initiated to defaults and need
/// only be set if supported and used.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag_Help
///
///@{
class PVRMediaTag : public DynamicCStructHdl<PVRMediaTag, PVR_MEDIA_TAG>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRMediaTag()
  {
    m_cStructure->iSeriesNumber = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodePartNumber = PVR_MEDIA_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iDuration = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iPriority = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreType = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreSubType = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iPlayCount = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iLastPlayedPosition = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->sectionType = PVR_MEDIA_TAG_SECTION_TYPE_UNKNOWN;
    m_cStructure->mediaType = PVR_MEDIA_TAG_TYPE_UNKNOWN;
    m_cStructure->sizeInBytes = PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE;
    m_cStructure->iClientProviderUid = PVR_PROVIDER_INVALID_UID;
  }
  PVRMediaTag(const PVRMediaTag& mediaTag) : DynamicCStructHdl(mediaTag) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **MediaTag id** | `std::string` | @ref PVRMediaTag::SetMediaTagId "SetMediaTagId" | @ref PVRMediaTag::GetMediaTagId "GetMediaTagId" | *required to set*
  /// | **Title** | `std::string` | @ref PVRMediaTag::SetTitle "SetTitle" | @ref PVRMediaTag::GetTitle "GetTitle" | *required to set*
  /// | **Episode name** | `std::string` | @ref PVRMediaTag::SetEpisodeName "SetEpisodeName" | @ref PVRMediaTag::GetEpisodeName "GetEpisodeName" | *optional*
  /// | **Series number** | `int` | @ref PVRMediaTag::SetSeriesNumber "SetSeriesNumber" | @ref PVRMediaTag::GetSeriesNumber "GetSeriesNumber" | *optional*
  /// | **Episode number** | `int` | @ref PVRMediaTag::SetEpisodeNumber "SetEpisodeNumber" | @ref PVRMediaTag::GetEpisodeNumber "GetEpisodeNumber" | *optional*
  /// | **Episode part number** | `int` | @ref PVRMediaTag::SetEpisodePartNumber "SetEpisodePartNumber" | @ref PVRMediaTag::GetEpisodePartNumber "GetEpisodePartNumber" | *optional*
  /// | **Year** | `int` | @ref PVRMediaTag::SetYear "SetYear" | @ref PVRMediaTag::GetYear "GetYear" | *optional*
  /// | **Directory** | `std::string` | @ref PVRMediaTag::SetDirectory "SetDirectory" | @ref PVRMediaTag::GetDirectory "GetDirectory" | *optional*
  /// | **Plot outline** | `std::string` | @ref PVRMediaTag::SetPlotOutline "SetPlotOutline" | @ref PVRMediaTag::GetPlotOutline "GetPlotOutline" | *optional*
  /// | **Plot** | `std::string` | @ref PVRMediaTag::SetPlot "SetPlot" | @ref PVRMediaTag::GetPlot "GetPlot" | *optional*
  /// | **Original title** | `std::string` | @ref PVRMediaTag::SetOriginalTitle "SetOriginalTitle" | @ref PVRMediaTag::GetOriginalTitle "GetOriginalTitle" | *optional*
  /// | **Cast** | `std::string` | @ref PVRMediaTag::SetCast "SetCast" | @ref PVRMediaTag::GetCast "GetCast" | *optional*
  /// | **Director** | `std::string` | @ref PVRMediaTag::SetDirector "SetDirector" | @ref PVRMediaTag::GetDirector "GetDirector" | *optional*
  /// | **Writer** | `std::string` | @ref PVRMediaTag::SetWriter "SetWriter" | @ref PVRMediaTag::GetWriter "GetWriter" | *optional*
  /// | **Genre description** | `std::string` | @ref PVRMediaTag::SetGenreDescription "SetGenreDescription" | @ref PVRMediaTag::GetGenreDescription "GetGenreDescription" | *optional*
  /// | **IMDB number** | `std::string` | @ref PVREPGTag::SetIMDBNumber "SetIMDBNumber" | @ref PVREPGTag::GetIMDBNumber "GetIMDBNumber" | *optional*
  /// | **Icon path** | `std::string` | @ref PVRMediaTag::SetIconPath "SetIconPath" | @ref PVRMediaTag::GetIconPath "GetIconPath" | *optional*
  /// | **Thumbnail path** | `std::string` | @ref PVRMediaTag::SetThumbnailPath "SetThumbnailPath" | @ref PVRMediaTag::GetThumbnailPath "GetThumbnailPath" | *optional*
  /// | **Fanart path** | `std::string` | @ref PVRMediaTag::SetFanartPath "SetFanartPath" | @ref PVRMediaTag::GetFanartPath "GetFanartPath" | *optional*
  /// | **MediaTag time** | `time_t` | @ref PVRMediaTag::SetMediaTagTime "SetMediaTagTime" | @ref PVRMediaTag::GetMediaTagTime "GetMediaTagTime" | *optional*
  /// | **Duration** | `int` | @ref PVRMediaTag::SetDuration "SetDuration" | @ref PVRMediaTag::GetDuration "GetDuration" | *optional*
  /// | **Priority** | `int` | @ref PVRMediaTag::SetPriority "SetPriority" | @ref PVRMediaTag::GetPriority "GetPriority" | *optional*
  /// | **Genre type** | `int` | @ref PVRMediaTag::SetGenreType "SetGenreType" | @ref PVRMediaTag::GetGenreType "GetGenreType" | *optional*
  /// | **Genre sub type** | `int` | @ref PVRMediaTag::SetGenreSubType "SetGenreSubType" | @ref PVRMediaTag::GetGenreSubType "GetGenreSubType" | *optional*
  /// | **Play count** | `int` | @ref PVRMediaTag::SetPlayCount "SetPlayCount" | @ref PVRMediaTag::GetPlayCount "GetPlayCount" | *optional*
  /// | **Last played position** | `int` | @ref PVRMediaTag::SetLastPlayedPosition "SetLastPlayedPosition" | @ref PVRMediaTag::GetLastPlayedPosition "GetLastPlayedPosition" | *optional*
  /// | **Is deleted** | `bool` | @ref PVRMediaTag::SetIsDeleted "SetIsDeleted" | @ref PVRMediaTag::GetIsDeleted "GetIsDeleted" | *optional*
  /// | **Section type** | @ref PVR_MEDIA_TAG_SECTION_TYPE | @ref PVRMediaTag::SetChannelType "SetChannelType" | @ref PVRMediaTag::GetChannelType "GetChannelType" | *optional*
  /// | **Media type** | @ref PVR_MEDIA_TAG_TYPE | @ref PVRMediaTag::SetMediaType "SetMediaType" | @ref PVRMediaTag::GetMediaType "GetMediaType" | *optional*
  /// | **First aired** | `std::string` | @ref PVRMediaTag::SetFirstAired "SetFirstAired" | @ref PVRMediaTag::GetFirstAired "GetFirstAired" | *optional*
  /// | **Flags** | `std::string` | @ref PVRMediaTag::SetFlags "SetFlags" | @ref PVRMediaTag::GetFlags "GetFlags" | *optional*
  /// | **Size in bytes** | `std::string` | @ref PVRMediaTag::SetSizeInBytes "SetSizeInBytes" | @ref PVRMediaTag::GetSizeInBytes "GetSizeInBytes" | *optional*
  /// | **Client provider unique identifier** | `int` | @ref PVRChannel::SetClientProviderUid "SetClientProviderUid" | @ref PVRTimer::GetClientProviderUid "GetClientProviderUid" | *optional*
  /// | **Provider name** | `std::string` | @ref PVRChannel::SetProviderName "SetProviderlName" | @ref PVRChannel::GetProviderName "GetProviderName" | *optional*
  /// | **Parental rating age** | `unsigned int` | @ref PVRMediaTag::SetParentalRating "SetParentalRating" | @ref PVRMediaTag::GetParentalRating "GetParentalRating" | *optional*
  /// | **Parental rating code** | `std::string` | @ref PVRMediaTag::SetParentalRatingCode "SetParentalRatingCode" | @ref PVRMediaTag::GetParentalRatingCode "GetParentalRatingCode" | *optional*
  /// | **Parental rating icon** | `std::string` | @ref PVRMediaTag::SetParentalRatingIcon "SetParentalRatingIcon" | @ref PVRMediaTag::GetParentalRatingIcon "GetParentalRatingIcon" | *optional*
  /// | **Parental rating source** | `std::string` | @ref PVRMediaTag::SetParentalRatingSource "SetParentalRatingSource" | @ref PVRMediaTag::GetParentalRatingSource "GetParentalRatingSource" | *optional*

  /// @addtogroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag
  ///@{

  /// @brief **required**\n
  /// Unique identifier of the media tag on the client.
  void SetMediaTagId(const std::string& mediaTagId)
  {
    ReallocAndCopyString(&m_cStructure->strMediaTagId, mediaTagId.c_str());
  }

  /// @brief To get with @ref SetMediaTagId changed values.
  std::string GetMediaTagId() const
  {
    return m_cStructure->strMediaTagId ? m_cStructure->strMediaTagId : "";
  }

  /// @brief **required**\n
  /// The title of this media tag.
  void SetTitle(const std::string& title)
  {
    ReallocAndCopyString(&m_cStructure->strTitle, title.c_str());
  }

  /// @brief To get with @ref SetTitle changed values.
  std::string GetTitle() const { return m_cStructure->strTitle ? m_cStructure->strTitle : ""; }

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
  /// Directory of this media tag on the client.
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
  /// Original title.
  void SetOriginalTitle(const std::string& originalTitle)
  {
    ReallocAndCopyString(&m_cStructure->strOriginalTitle, originalTitle.c_str());
  }

  /// @brief To get with @ref SetOriginalTitle changed values
  std::string GetOriginalTitle() const
  {
    return m_cStructure->strOriginalTitle ? m_cStructure->strOriginalTitle : "";
  }

  /// @brief **optional**\n
  /// Cast name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetCast(const std::string& cast)
  {
    ReallocAndCopyString(&m_cStructure->strCast, cast.c_str());
  }

  /// @brief To get with @ref SetCast changed values
  std::string GetCast() const { return m_cStructure->strCast ? m_cStructure->strCast : ""; }

  /// @brief **optional**\n
  /// Director name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetDirector(const std::string& director)
  {
    ReallocAndCopyString(&m_cStructure->strDirector, director.c_str());
  }

  /// @brief To get with @ref SetDirector changed values.
  std::string GetDirector() const
  {
    return m_cStructure->strDirector ? m_cStructure->strDirector : "";
  }

  /// @brief **optional**\n
  /// Writer name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetWriter(const std::string& writer)
  {
    ReallocAndCopyString(&m_cStructure->strWriter, writer.c_str());
  }

  /// @brief To get with @ref SetDirector changed values
  std::string GetWriter() const { return m_cStructure->strWriter ? m_cStructure->strWriter : ""; }

  /// @brief **optional**\n
  /// [IMDB](https://en.wikipedia.org/wiki/IMDb) identification number.
  void SetIMDBNumber(const std::string& IMDBNumber)
  {
    ReallocAndCopyString(&m_cStructure->strIMDBNumber, IMDBNumber.c_str());
  }

  /// @brief To get with @ref SetIMDBNumber changed values.
  std::string GetIMDBNumber() const
  {
    return m_cStructure->strIMDBNumber ? m_cStructure->strIMDBNumber : "";
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
  /// Start time of the media tag.
  void SetMediaTagTime(time_t mediaTagTime) { m_cStructure->mediaTagTime = mediaTagTime; }

  /// @brief To get with @ref SetMediaTagTime changed values.
  time_t GetMediaTagTime() const { return m_cStructure->mediaTagTime; }

  /// @brief **optional**\n
  /// Duration of the media tag in seconds.
  void SetDuration(int duration) { m_cStructure->iDuration = duration; }

  /// @brief To get with @ref SetDuration changed values.
  int GetDuration() const { return m_cStructure->iDuration; }

  /// @brief **optional**\n
  /// Priority of this media tag (from 0 - 100).
  void SetPriority(int priority) { m_cStructure->iPriority = priority; }

  /// @brief To get with @ref SetPriority changed values.
  int GetPriority() const { return m_cStructure->iPriority; }

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
  /// kodi::addon::PVRMediaTag tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 2** (in case of other, not ETSI EN 300 468 conform genre types):
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRMediaTag tag;
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
  /// kodi::addon::PVRMediaTag tag;
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
  /// kodi::addon::PVRMediaTag tag;
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
  /// Play count of this media tag on the client.
  void SetPlayCount(int playCount) { m_cStructure->iPlayCount = playCount; }

  /// @brief To get with @ref SetPlayCount changed values.
  int GetPlayCount() const { return m_cStructure->iPlayCount; }

  /// @brief **optional**\n
  /// Last played position of this media tag on the client.
  void SetLastPlayedPosition(int lastPlayedPosition)
  {
    m_cStructure->iLastPlayedPosition = lastPlayedPosition;
  }

  /// @brief To get with @ref SetLastPlayedPosition changed values.
  int GetLastPlayedPosition() const { return m_cStructure->iLastPlayedPosition; }

  /// @brief **optional**\n
  /// Section type.
  ///
  /// Set to @ref PVR_MEDIA_TAG_SECTION_TYPE_UNKNOWN if the type cannot be
  /// determined.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRMediaTag tag;
  /// tag.SetSectionType(PVR_MEDIA_TAG_SECTION_TYPE_TV);
  /// ~~~~~~~~~~~~~
  ///
  void SetSectionType(PVR_MEDIA_TAG_SECTION_TYPE sectionType)
  {
    m_cStructure->sectionType = sectionType;
  }

  /// @brief To get with @ref SetSectionType changed values
  PVR_MEDIA_TAG_SECTION_TYPE GetSectionType() const { return m_cStructure->sectionType; }

  /// @brief **optional**\n
  /// Media type.
  ///
  /// Set to @ref PVR_MEDIA_TAG_TYPE_UNKNOWN if the type cannot be
  /// determined. Each media belongs to a media class.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRMediaTag tag;
  /// tag.SetMediaType(PVR_MEDIA_TAG_TYPE_VIDEO);
  /// ~~~~~~~~~~~~~
  ///
  void SetMediaType(PVR_MEDIA_TAG_TYPE mediaType) { m_cStructure->mediaType = mediaType; }

  /// @brief To get with @ref SetMediaType changed values
  PVR_MEDIA_TAG_TYPE GetMediaType() const { return m_cStructure->mediaType; }

  /// @brief **optional**\n
  /// First aired date of this media tag.
  ///
  /// Used only for display purposes. Specify in W3C date format "YYYY-MM-DD".
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRMediaTag tag;
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
  /// Bit field of independent flags associated with the media tag.
  ///
  /// See @ref cpp_kodi_addon_pvr_Defs_MediaTag_PVR_MEDIA_TAG_FLAG for
  /// available bit flags.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_MediaTag_PVR_MEDIA_TAG_FLAG
  ///
  void SetFlags(unsigned int flags) { m_cStructure->iFlags = flags; }

  /// @brief To get with @ref SetFlags changed values.
  unsigned int GetFlags() const { return m_cStructure->iFlags; }

  /// @brief **optional**\n
  /// Size of the media tag in bytes.
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
  /// Age rating for the media tag.
  void SetParentalRating(unsigned int iParentalRating)
  {
    m_cStructure->iParentalRating = iParentalRating;
  }

  /// @brief To get with @ref SetParentalRating changed values
  unsigned int GetParentalRating() const { return m_cStructure->iParentalRating; }

  /// @brief **optional**\n
  /// Parental rating code for this media tag.
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
  /// Parental rating icon for this media tag.
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
  /// Parental rating source for this media tag.
  void SetParentalRatingSource(const std::string& ratingSource)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingSource, ratingSource.c_str());
  }

  /// @brief To get with @ref SetParentalRatingSource changed values.
  std::string GetParentalRatingSource() const
  {
    return m_cStructure->strParentalRatingSource ? m_cStructure->strParentalRatingSource : "";
  }

  static void AllocResources(const PVR_MEDIA_TAG* source, PVR_MEDIA_TAG* target)
  {
    target->strMediaTagId = AllocAndCopyString(source->strMediaTagId);
    target->strTitle = AllocAndCopyString(source->strTitle);
    target->strEpisodeName = AllocAndCopyString(source->strEpisodeName);
    target->strDirectory = AllocAndCopyString(source->strDirectory);
    target->strPlotOutline = AllocAndCopyString(source->strPlotOutline);
    target->strPlot = AllocAndCopyString(source->strPlot);
    target->strOriginalTitle = AllocAndCopyString(source->strOriginalTitle);
    target->strCast = AllocAndCopyString(source->strCast);
    target->strDirector = AllocAndCopyString(source->strDirector);
    target->strWriter = AllocAndCopyString(source->strWriter);
    target->strIMDBNumber = AllocAndCopyString(source->strIMDBNumber);
    target->strGenreDescription = AllocAndCopyString(source->strGenreDescription);
    target->strIconPath = AllocAndCopyString(source->strIconPath);
    target->strThumbnailPath = AllocAndCopyString(source->strThumbnailPath);
    target->strFanartPath = AllocAndCopyString(source->strFanartPath);
    target->strFirstAired = AllocAndCopyString(source->strFirstAired);
    target->strProviderName = AllocAndCopyString(source->strProviderName);
    target->strParentalRatingCode = AllocAndCopyString(source->strParentalRatingCode);
    target->strParentalRatingIcon = AllocAndCopyString(source->strParentalRatingIcon);
    target->strParentalRatingSource = AllocAndCopyString(source->strParentalRatingSource);
  }

  static void FreeResources(PVR_MEDIA_TAG* target)
  {
    FreeString(target->strMediaTagId);
    FreeString(target->strTitle);
    FreeString(target->strEpisodeName);
    FreeString(target->strDirectory);
    FreeString(target->strPlotOutline);
    FreeString(target->strPlot);
    FreeString(target->strOriginalTitle);
    FreeString(target->strCast);
    FreeString(target->strDirector);
    FreeString(target->strWriter);
    FreeString(target->strIMDBNumber);
    FreeString(target->strGenreDescription);
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
  PVRMediaTag(const PVR_MEDIA_TAG* mediaTag) : DynamicCStructHdl(mediaTag) {}
  PVRMediaTag(PVR_MEDIA_TAG* mediaTag) : DynamicCStructHdl(mediaTag) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTagsResultSet class PVRMediaTagsResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag
/// @brief **PVR add-on mediaTag transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetMediaTags().
///
/// @note This becomes only be used on addon call above, not usable outside on
/// addon itself.
///@{
class PVRMediaTagsResultSet
{
public:
  /*! \cond PRIVATE */
  PVRMediaTagsResultSet() = delete;
  PVRMediaTagsResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTagsResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRMediaTag& tag)
  {
    m_instance->toKodi->TransferMediaTagEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
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
