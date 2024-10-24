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
// "C++" Definitions group 4 - PVR EPG
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTag class PVREPGTag
/// @ingroup cpp_kodi_addon_pvr_Defs_epg
/// @brief **PVR add-on EPG data tag**\n
/// Representation of an EPG event.
///
/// Herewith all EPG related data are saved in one class whereby the data can
/// be exchanged with Kodi, or can also be used on the addon to save there.
///
/// See @ref cpp_kodi_addon_pvr_EPGTag "EPG methods" about usage.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_epg_PVREPGTag_Help
///
///@{
class PVREPGTag : public DynamicCStructHdl<PVREPGTag, EPG_TAG>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVREPGTag()
  {
    m_cStructure->iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  }
  PVREPGTag(const PVREPGTag& epg) : DynamicCStructHdl(epg) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTag_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTag
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|---------
  /// | **Unique broadcast id** | `unsigned int` | @ref PVREPGTag::SetUniqueBroadcastId "SetUniqueBroadcastId" | @ref PVREPGTag::GetUniqueBroadcastId "GetUniqueBroadcastId" | *required to set*
  /// | **Unique channel id** | `unsigned int` | @ref PVREPGTag::SetUniqueChannelId "SetUniqueChannelId" | @ref PVREPGTag::GetUniqueChannelId "GetUniqueChannelId" | *required to set*
  /// | **Title** | `std::string` | @ref PVREPGTag::SetTitle "SetTitle" | @ref PVREPGTag::GetTitle "GetTitle" | *required to set*
  /// | **Title extra info** | `std::string` | @ref PVREPGTag::SetTitleExtraInfo "SetTitleExtraInfo" | @ref PVREPGTag::GetTitleExtraInfo "GetTitleExtraInfo" | *optional*
  /// | **Start time** | `time_t` | @ref PVREPGTag::SetStartTime "SetStartTime" | @ref PVREPGTag::GetStartTime "GetStartTime" | *required to set*
  /// | **End time** | `time_t` | @ref PVREPGTag::SetEndTime "SetEndTime" | @ref PVREPGTag::GetEndTime "GetEndTime" | *required to set*
  /// | **Plot outline** | `std::string` | @ref PVREPGTag::SetPlotOutline "SetPlotOutline" | @ref PVREPGTag::GetPlotOutline "GetPlotOutline" | *optional*
  /// | **Plot** | `std::string` | @ref PVREPGTag::SetPlot "SetPlot" | @ref PVREPGTag::GetPlot "GetPlot" | *optional*
  /// | **Original title** | `std::string` | @ref PVREPGTag::SetOriginalTitle "SetOriginalTitle" | @ref PVREPGTag::GetOriginalTitle "GetOriginalTitle" | *optional*
  /// | **Cast** | `std::string` | @ref PVREPGTag::SetCast "SetCast" | @ref PVREPGTag::GetCast "GetCast" | *optional*
  /// | **Director** | `std::string` | @ref PVREPGTag::SetDirector "SetDirector" | @ref PVREPGTag::GetDirector "GetDirector" | *optional*
  /// | **Writer** | `std::string` | @ref PVREPGTag::SetWriter "SetWriter" | @ref PVREPGTag::GetWriter "GetWriter" | *optional*
  /// | **Year** | `int` | @ref PVREPGTag::SetYear "SetYear" | @ref PVREPGTag::GetYear "GetYear" | *optional*
  /// | **IMDB number** | `std::string` | @ref PVREPGTag::SetIMDBNumber "SetIMDBNumber" | @ref PVREPGTag::GetIMDBNumber "GetIMDBNumber" | *optional*
  /// | **Icon path** | `std::string` | @ref PVREPGTag::SetIconPath "SetIconPath" | @ref PVREPGTag::GetIconPath "GetIconPath" | *optional*
  /// | **Genre type** | `int` | @ref PVREPGTag::SetGenreType "SetGenreType" | @ref PVREPGTag::GetGenreType "GetGenreType" | *optional*
  /// | **Genre sub type** | `int` | @ref PVREPGTag::SetGenreSubType "SetGenreSubType" | @ref PVREPGTag::GetGenreSubType "GetGenreSubType" | *optional*
  /// | **Genre description** | `std::string` | @ref PVREPGTag::SetGenreDescription "SetGenreDescription" | @ref PVREPGTag::GetGenreDescription "GetGenreDescription" | *optional*
  /// | **First aired** | `time_t` | @ref PVREPGTag::SetFirstAired "SetFirstAired" | @ref PVREPGTag::GetFirstAired "GetFirstAired" | *optional*
  /// | **Parental rating** | `unsigned int` | @ref PVREPGTag::SetParentalRating "SetParentalRating" | @ref PVREPGTag::GetParentalRating "GetParentalRating" | *optional*
  /// | **Parental rating code** | `std::string` | @ref PVREPGTag::SetParentalRatingCode "SetParentalRatingCode" | @ref PVREPGTag::GetParentalRatingCode "GetParentalRatingCode" | *optional*
  /// | **Parental rating icon** | `std::string` | @ref PVREPGTag::SetParentalRatingIcon "SetParentalRatingIcon" | @ref PVREPGTag::GetParentalRatingIcon "GetParentalRatingIcon" | *optional*
  /// | **Parental rating source** | `std::string` | @ref PVREPGTag::SetParentalRatingSource "SetParentalRatingSource" | @ref PVREPGTag::GetParentalRatingSource "GetParentalRatingSource" | *optional*
  /// | **Star rating** | `int` | @ref PVREPGTag::SetStarRating "SetStarRating" | @ref PVREPGTag::GetStarRating "GetStarRating" | *optional*
  /// | **Series number** | `int` | @ref PVREPGTag::SetSeriesNumber "SetSeriesNumber" | @ref PVREPGTag::GetSeriesNumber "GetSeriesNumber" | *optional*
  /// | **Episode number** | `int` | @ref PVREPGTag::SetEpisodeNumber "SetEpisodeNumber" | @ref PVREPGTag::GetEpisodeNumber "GetEpisodeNumber" | *optional*
  /// | **Episode part number** | `int` | @ref PVREPGTag::SetEpisodePartNumber "SetEpisodePartNumber" | @ref PVREPGTag::GetEpisodePartNumber "GetEpisodePartNumber" | *optional*
  /// | **Episode name** | `std::string` | @ref PVREPGTag::SetEpisodeName "SetEpisodeName" | @ref PVREPGTag::GetEpisodeName "GetEpisodeName" | *optional*
  /// | **Flags** | `unsigned int` | @ref PVREPGTag::SetFlags "SetFlags" | @ref PVREPGTag::GetFlags "GetFlags" | *optional*
  /// | **Series link** | `std::string` | @ref PVREPGTag::SetSeriesLink "SetSeriesLink" | @ref PVREPGTag::GetSeriesLink "GetSeriesLink" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTag
  ///@{

  /// @brief **required**\n
  /// Identifier for this event. Event uids must be unique for a channel. Valid uids must be greater than @ref EPG_TAG_INVALID_UID.
  void SetUniqueBroadcastId(unsigned int uniqueBroadcastId)
  {
    m_cStructure->iUniqueBroadcastId = uniqueBroadcastId;
  }

  /// @brief To get with @ref SetUniqueBroadcastId changed values.
  unsigned int GetUniqueBroadcastId() const { return m_cStructure->iUniqueBroadcastId; }

  /// @brief **required**\n
  /// Unique identifier of the channel this event belongs to.
  void SetUniqueChannelId(unsigned int uniqueChannelId)
  {
    m_cStructure->iUniqueChannelId = uniqueChannelId;
  }

  /// @brief To get with @ref SetUniqueChannelId changed values
  unsigned int GetUniqueChannelId() const { return m_cStructure->iUniqueChannelId; }

  /// @brief **required**\n
  /// This event's title.
  void SetTitle(const std::string& title)
  {
    ReallocAndCopyString(&m_cStructure->strTitle, title.c_str());
  }

  /// @brief To get with @ref SetTitle changed values.
  std::string GetTitle() const { return m_cStructure->strTitle ? m_cStructure->strTitle : ""; }

  /// @brief **optional**\n
  /// This event's title extra information.
  void SetTitleExtraInfo(const std::string& titleExtraInfo)
  {
    ReallocAndCopyString(&m_cStructure->strTitleExtraInfo, titleExtraInfo.c_str());
  }

  /// @brief To get with @ref SetTitleExtraInfo changed values.
  std::string GetTitleExtraInfo() const
  {
    return m_cStructure->strTitleExtraInfo ? m_cStructure->strTitleExtraInfo : "";
  }

  /// @brief **required**\n
  /// Start time in UTC.
  ///
  /// Seconds elapsed since 00:00 hours, Jan 1, 1970 UTC.
  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }

  /// @brief To get with @ref SetStartTime changed values.
  time_t GetStartTime() const { return m_cStructure->startTime; }

  /// @brief **required**\n
  /// End time in UTC.
  ///
  /// Seconds elapsed since 00:00 hours, Jan 1, 1970 UTC.
  void SetEndTime(time_t endTime) { m_cStructure->endTime = endTime; }

  /// @brief To get with @ref SetEndTime changed values.
  time_t GetEndTime() const { return m_cStructure->endTime; }

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

  /// @brief To get with @ref GetPlot changed values.
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
  /// Year.
  void SetYear(int year) { m_cStructure->iYear = year; }

  /// @brief To get with @ref SetYear changed values.
  int GetYear() const { return m_cStructure->iYear; }

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
  /// Icon path.
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
  /// Genre type.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails EPG_EVENT_CONTENTMASK
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
  /// kodi::addon::PVREPGTag tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 2** (in case of other, not ETSI EN 300 468 conform genre types):
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVREPGTag tag;
  /// tag.SetGenreType(EPG_GENRE_USE_STRING);
  /// tag.SetGenreDescription("My special genre name"); // Should use (if possible) kodi::GetLocalizedString(...) to have match user language.
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }

  /// @brief To get with @ref SetGenreType changed values
  int GetGenreType() const { return m_cStructure->iGenreType; }

  /// @brief **optional**\n
  /// Genre sub type.
  ///
  /// @copydetails EPG_EVENT_CONTENTMASK
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
  /// kodi::addon::PVREPGTag tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE);
  /// tag.SetGenreSubType(EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_JAZZ);
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }

  /// @brief To get with @ref SetGenreSubType changed values.
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  /// @brief **optional**\n genre. Will be used only when genreType == @ref EPG_GENRE_USE_STRING
  /// or genreSubType == @ref EPG_GENRE_USE_STRING.
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
  /// kodi::addon::PVREPGTag tag;
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
  /// First aired in UTC.
  void SetFirstAired(const std::string& firstAired)
  {
    ReallocAndCopyString(&m_cStructure->strFirstAired, firstAired.c_str());
  }

  /// @brief To get with @ref SetFirstAired changed values.
  std::string GetFirstAired() const
  {
    return m_cStructure->strFirstAired ? m_cStructure->strFirstAired : "";
  }

  /// @brief **optional**\n
  /// Parental rating.
  void SetParentalRating(unsigned int parentalRating)
  {
    m_cStructure->iParentalRating = parentalRating;
  }

  /// @brief To get with @ref SetParentalRating changed values.
  unsigned int GetParentalRating() const { return m_cStructure->iParentalRating; }

  /// @brief **optional**\n
  /// This event's parental rating code.
  void SetParentalRatingCode(const std::string& parentalRatingCode)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingCode, parentalRatingCode.c_str());
  }

  /// @brief To get with @ref SetParentalRatingCode changed values.
  std::string GetParentalRatingCode() const
  {
    return m_cStructure->strParentalRatingCode ? m_cStructure->strParentalRatingCode : "";
  }

  /// @brief **optional**\n
  /// This event's parental rating icon.
  void SetParentalRatingIcon(const std::string& parentalRatingIcon)
  {
    ReallocAndCopyString(&m_cStructure->strParentalRatingIcon, parentalRatingIcon.c_str());
  }

  /// @brief To get with @ref SetParentalRatingIcon changed values.
  std::string GetParentalRatingIcon() const
  {
    return m_cStructure->strParentalRatingIcon ? m_cStructure->strParentalRatingIcon : "";
  }

  /// @brief **optional**\n
  /// The event's parental rating source.
  void SetParentalRatingSource(const std::string& parentalRatingSource)
  {
    //m_parentalRatingSource = parentalRatingSource;
    ReallocAndCopyString(&m_cStructure->strParentalRatingSource, parentalRatingSource.c_str());
  }

  /// @brief To get with @ref SetParentalRatingSource changed values.
  std::string GetParentalRatingSource() const
  {
    return m_cStructure->strParentalRatingSource ? m_cStructure->strParentalRatingSource : "";
  }

  /// @brief **optional**\n
  /// Star rating.
  void SetStarRating(int starRating) { m_cStructure->iStarRating = starRating; }

  /// @brief To get with @ref SetStarRating changed values.
  int GetStarRating() const { return m_cStructure->iStarRating; }

  /// @brief **optional**\n
  /// Series number.
  void SetSeriesNumber(int seriesNumber) { m_cStructure->iSeriesNumber = seriesNumber; }

  /// @brief To get with @ref SetSeriesNumber changed values.
  int GetSeriesNumber() const { return m_cStructure->iSeriesNumber; }

  /// @brief **optional**\n
  /// Episode number.
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
  /// Episode name.
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
  /// Bit field of independent flags associated with the EPG entry.
  ///
  /// See @ref cpp_kodi_addon_pvr_Defs_epg_EPG_TAG_FLAG for available bit flags.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_epg_EPG_TAG_FLAG
  ///
  void SetFlags(unsigned int flags) { m_cStructure->iFlags = flags; }

  /// @brief To get with @ref SetFlags changed values.
  unsigned int GetFlags() const { return m_cStructure->iFlags; }

  /// @brief **optional**\n
  /// Series link for this event.
  void SetSeriesLink(const std::string& seriesLink)
  {
    ReallocAndCopyString(&m_cStructure->strSeriesLink, seriesLink.c_str());
  }

  /// @brief To get with @ref SetSeriesLink changed values.
  std::string GetSeriesLink() const
  {
    return m_cStructure->strSeriesLink ? m_cStructure->strSeriesLink : "";
  }

  ///@}

  static void AllocResources(const EPG_TAG* source, EPG_TAG* target)
  {
    target->strTitle = AllocAndCopyString(source->strTitle);
    target->strTitleExtraInfo = AllocAndCopyString(source->strTitleExtraInfo);
    target->strPlotOutline = AllocAndCopyString(source->strPlotOutline);
    target->strPlot = AllocAndCopyString(source->strPlot);
    target->strOriginalTitle = AllocAndCopyString(source->strOriginalTitle);
    target->strCast = AllocAndCopyString(source->strCast);
    target->strDirector = AllocAndCopyString(source->strDirector);
    target->strWriter = AllocAndCopyString(source->strWriter);
    target->strIMDBNumber = AllocAndCopyString(source->strIMDBNumber);
    target->strIconPath = AllocAndCopyString(source->strIconPath);
    target->strGenreDescription = AllocAndCopyString(source->strGenreDescription);
    target->strParentalRatingCode = AllocAndCopyString(source->strParentalRatingCode);
    target->strParentalRatingIcon = AllocAndCopyString(source->strParentalRatingIcon);
    target->strParentalRatingSource = AllocAndCopyString(source->strParentalRatingSource);
    target->strEpisodeName = AllocAndCopyString(source->strEpisodeName);
    target->strSeriesLink = AllocAndCopyString(source->strSeriesLink);
    target->strFirstAired = AllocAndCopyString(source->strFirstAired);
  }

  static void FreeResources(EPG_TAG* target)
  {
    FreeString(target->strTitle);
    FreeString(target->strTitleExtraInfo);
    FreeString(target->strPlotOutline);
    FreeString(target->strPlot);
    FreeString(target->strOriginalTitle);
    FreeString(target->strCast);
    FreeString(target->strDirector);
    FreeString(target->strWriter);
    FreeString(target->strIMDBNumber);
    FreeString(target->strIconPath);
    FreeString(target->strGenreDescription);
    FreeString(target->strParentalRatingCode);
    FreeString(target->strParentalRatingIcon);
    FreeString(target->strParentalRatingSource);
    FreeString(target->strEpisodeName);
    FreeString(target->strSeriesLink);
    FreeString(target->strFirstAired);
  }

private:
  PVREPGTag(const EPG_TAG* epg) : DynamicCStructHdl(epg) {}
  PVREPGTag(EPG_TAG* epg) : DynamicCStructHdl(epg) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTagsResultSet class PVREPGTagsResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTag
/// @brief **PVR add-on EPG entry transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetEPGForChannel().
///
/// @note This becomes only be used on addon call above, not usable outside on
/// addon itself.
///@{
class PVREPGTagsResultSet
{
public:
  /*! \cond PRIVATE */
  PVREPGTagsResultSet() = delete;
  PVREPGTagsResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_epg_PVREPGTagsResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVREPGTag& tag)
  {
    m_instance->toKodi->TransferEpgEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
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
