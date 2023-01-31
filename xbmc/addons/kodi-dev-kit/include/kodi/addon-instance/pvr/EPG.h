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
class PVREPGTag : public CStructHdl<PVREPGTag, EPG_TAG>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVREPGTag()
  {
    memset(m_cStructure, 0, sizeof(EPG_TAG));
    m_cStructure->iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  }
  PVREPGTag(const PVREPGTag& epg)
    : CStructHdl(epg),
      m_title(epg.m_title),
      m_plotOutline(epg.m_plotOutline),
      m_plot(epg.m_plot),
      m_originalTitle(epg.m_originalTitle),
      m_cast(epg.m_cast),
      m_director(epg.m_director),
      m_writer(epg.m_writer),
      m_IMDBNumber(epg.m_IMDBNumber),
      m_episodeName(epg.m_episodeName),
      m_iconPath(epg.m_iconPath),
      m_seriesLink(epg.m_seriesLink),
      m_genreDescription(epg.m_genreDescription),
      m_parentalRatingCode(epg.m_parentalRatingCode),
      m_firstAired(epg.m_firstAired)
  {
  }
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
  /// | **Parental rating** | `int` | @ref PVREPGTag::SetParentalRating "SetParentalRating" | @ref PVREPGTag::GetParentalRating "GetParentalRating" | *optional*
  /// | **Parental rating code** | `int` | @ref PVREPGTag::SetParentalRatingCode "SetParentalRatingCode" | @ref PVREPGTag::GetParentalRatingCode "GetParentalRatingCode" | *optional*
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
  void SetTitle(const std::string& title) { m_title = title; }

  /// @brief To get with @ref SetTitle changed values.
  std::string GetTitle() const { return m_title; }

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
  void SetPlotOutline(const std::string& plotOutline) { m_plotOutline = plotOutline; }

  /// @brief To get with @ref SetPlotOutline changed values.
  std::string GetPlotOutline() const { return m_plotOutline; }

  /// @brief **optional**\n
  /// Plot name.
  void SetPlot(const std::string& plot) { m_plot = plot; }

  /// @brief To get with @ref GetPlot changed values.
  std::string GetPlot() const { return m_plot; }

  /// @brief **optional**\n
  /// Original title.
  void SetOriginalTitle(const std::string& originalTitle) { m_originalTitle = originalTitle; }

  /// @brief To get with @ref SetOriginalTitle changed values
  std::string GetOriginalTitle() const { return m_originalTitle; }

  /// @brief **optional**\n
  /// Cast name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetCast(const std::string& cast) { m_cast = cast; }

  /// @brief To get with @ref SetCast changed values
  std::string GetCast() const { return m_cast; }

  /// @brief **optional**\n
  /// Director name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetDirector(const std::string& director) { m_director = director; }

  /// @brief To get with @ref SetDirector changed values.
  std::string GetDirector() const { return m_director; }

  /// @brief **optional**\n
  /// Writer name(s).
  ///
  /// @note Use @ref EPG_STRING_TOKEN_SEPARATOR to separate different persons.
  void SetWriter(const std::string& writer) { m_writer = writer; }

  /// @brief To get with @ref SetDirector changed values
  std::string GetWriter() const { return m_writer; }

  /// @brief **optional**\n
  /// Year.
  void SetYear(int year) { m_cStructure->iYear = year; }

  /// @brief To get with @ref SetYear changed values.
  int GetYear() const { return m_cStructure->iYear; }

  /// @brief **optional**\n
  /// [IMDB](https://en.wikipedia.org/wiki/IMDb) identification number.
  void SetIMDBNumber(const std::string& IMDBNumber) { m_IMDBNumber = IMDBNumber; }

  /// @brief To get with @ref SetIMDBNumber changed values.
  std::string GetIMDBNumber() const { return m_IMDBNumber; }

  /// @brief **optional**\n
  /// Icon path.
  void SetIconPath(const std::string& iconPath) { m_iconPath = iconPath; }

  /// @brief To get with @ref SetIconPath changed values.
  std::string GetIconPath() const { return m_iconPath; }

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
    m_genreDescription = genreDescription;
  }

  /// @brief To get with @ref SetGenreDescription changed values.
  std::string GetGenreDescription() const { return m_genreDescription; }

  /// @brief **optional**\n
  /// First aired in UTC.
  void SetFirstAired(const std::string& firstAired) { m_firstAired = firstAired; }

  /// @brief To get with @ref SetFirstAired changed values.
  std::string GetFirstAired() const { return m_firstAired; }

  /// @brief **optional**\n
  /// Parental rating.
  void SetParentalRating(int parentalRating) { m_cStructure->iParentalRating = parentalRating; }

  /// @brief To get with @ref SetParentalRatinge changed values.
  int GetParentalRating() const { return m_cStructure->iParentalRating; }

  /// @brief **required**\n
  /// This event's parental rating code.
  void SetParentalRatingCode(const std::string& parentalRatingCode)
  {
    m_parentalRatingCode = parentalRatingCode;
  }

  /// @brief To get with @ref SetParentalRatingCode changed values.
  std::string GetParentalRatingCode() const { return m_parentalRatingCode; }

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
  void SetEpisodeName(const std::string& episodeName) { m_episodeName = episodeName; }

  /// @brief To get with @ref SetEpisodeName changed values.
  std::string GetEpisodeName() const { return m_episodeName; }

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
  void SetSeriesLink(const std::string& seriesLink) { m_seriesLink = seriesLink; }

  /// @brief To get with @ref SetSeriesLink changed values.
  std::string GetSeriesLink() const { return m_seriesLink; }

  ///@}

  // Internal used, as this have own memory for strings and to translate them to "C"
  EPG_TAG* GetTag() const
  {
    m_cStructure->strTitle = m_title.c_str();
    m_cStructure->strPlotOutline = m_plotOutline.c_str();
    m_cStructure->strPlot = m_plot.c_str();
    m_cStructure->strOriginalTitle = m_originalTitle.c_str();
    m_cStructure->strCast = m_cast.c_str();
    m_cStructure->strDirector = m_director.c_str();
    m_cStructure->strWriter = m_writer.c_str();
    m_cStructure->strIMDBNumber = m_IMDBNumber.c_str();
    m_cStructure->strIconPath = m_iconPath.c_str();
    m_cStructure->strGenreDescription = m_genreDescription.c_str();
    m_cStructure->strParentalRatingCode = m_parentalRatingCode.c_str();
    m_cStructure->strEpisodeName = m_episodeName.c_str();
    m_cStructure->strSeriesLink = m_seriesLink.c_str();
    m_cStructure->strFirstAired = m_firstAired.c_str();

    return m_cStructure;
  }

private:
  PVREPGTag(const EPG_TAG* epg) : CStructHdl(epg) { SetData(epg); }
  PVREPGTag(EPG_TAG* epg) : CStructHdl(epg) { SetData(epg); }

  const PVREPGTag& operator=(const PVREPGTag& right);
  const PVREPGTag& operator=(const EPG_TAG& right);
  operator EPG_TAG*();

  std::string m_title;
  std::string m_plotOutline;
  std::string m_plot;
  std::string m_originalTitle;
  std::string m_cast;
  std::string m_director;
  std::string m_writer;
  std::string m_IMDBNumber;
  std::string m_episodeName;
  std::string m_iconPath;
  std::string m_seriesLink;
  std::string m_genreDescription;
  std::string m_parentalRatingCode;
  std::string m_firstAired;

  void SetData(const EPG_TAG* tag)
  {
    m_title = tag->strTitle == nullptr ? "" : tag->strTitle;
    m_plotOutline = tag->strPlotOutline == nullptr ? "" : tag->strPlotOutline;
    m_plot = tag->strPlot == nullptr ? "" : tag->strPlot;
    m_originalTitle = tag->strOriginalTitle == nullptr ? "" : tag->strOriginalTitle;
    m_cast = tag->strCast == nullptr ? "" : tag->strCast;
    m_director = tag->strDirector == nullptr ? "" : tag->strDirector;
    m_writer = tag->strWriter == nullptr ? "" : tag->strWriter;
    m_IMDBNumber = tag->strIMDBNumber == nullptr ? "" : tag->strIMDBNumber;
    m_iconPath = tag->strIconPath == nullptr ? "" : tag->strIconPath;
    m_genreDescription = tag->strGenreDescription == nullptr ? "" : tag->strGenreDescription;
    m_parentalRatingCode = tag->strParentalRatingCode == nullptr ? "" : tag->strParentalRatingCode;
    m_episodeName = tag->strEpisodeName == nullptr ? "" : tag->strEpisodeName;
    m_seriesLink = tag->strSeriesLink == nullptr ? "" : tag->strSeriesLink;
    m_firstAired = tag->strFirstAired == nullptr ? "" : tag->strFirstAired;
  }
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
    m_instance->toKodi->TransferEpgEntry(m_instance->toKodi->kodiInstance, m_handle, tag.GetTag());
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
