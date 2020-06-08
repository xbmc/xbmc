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

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVREPGTag : public CStructHdl<PVREPGTag, EPG_TAG>
{
public:
  PVREPGTag()
  {
    memset(m_cStructure, 0, sizeof(EPG_TAG));
    m_cStructure->iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  }
  PVREPGTag(const PVREPGTag& epg) : CStructHdl(epg)
  {
    m_title = epg.m_title;
    m_plotOutline = epg.m_plotOutline;
    m_plot = epg.m_plot;
    m_originalTitle = epg.m_originalTitle;
    m_cast = epg.m_cast;
    m_director = epg.m_director;
    m_writer = epg.m_writer;
    m_IMDBNumber = epg.m_IMDBNumber;
    m_iconPath = epg.m_iconPath;
    m_genreDescription = epg.m_genreDescription;
    m_episodeName = epg.m_episodeName;
    m_seriesLink = epg.m_seriesLink;
    m_firstAired = epg.m_firstAired;
  }
  PVREPGTag(const EPG_TAG* epg) : CStructHdl(epg) { SetData(epg); }
  PVREPGTag(EPG_TAG* epg) : CStructHdl(epg) { SetData(epg); }

  void SetUniqueBroadcastId(unsigned int uniqueBroadcastId)
  {
    m_cStructure->iUniqueBroadcastId = uniqueBroadcastId;
  }
  unsigned int GetUniqueBroadcastId() const { return m_cStructure->iUniqueBroadcastId; }

  void SetUniqueChannelId(unsigned int uniqueChannelId)
  {
    m_cStructure->iUniqueChannelId = uniqueChannelId;
  }
  unsigned int GetUniqueChannelId() const { return m_cStructure->iUniqueChannelId; }

  void SetTitle(const std::string& title) { m_title = title; }
  std::string GetTitle() const { return m_title; }

  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }
  time_t GetStartTime() const { return m_cStructure->startTime; }

  void SetEndTime(time_t endTime) { m_cStructure->endTime = endTime; }
  time_t GetEndTime() const { return m_cStructure->endTime; }

  void SetPlotOutline(const std::string& plotOutline) { m_plotOutline = plotOutline; }
  std::string GetPlotOutline() const { return m_plotOutline; }

  void SetPlot(const std::string& plot) { m_plot = plot; }
  std::string GetPlot() const { return m_plot; }

  void SetOriginalTitle(const std::string& originalTitle) { m_originalTitle = originalTitle; }
  std::string GetOriginalTitle() const { return m_originalTitle; }

  void SetCast(const std::string& cast) { m_cast = cast; }
  std::string GetCast() const { return m_cast; }

  void SetDirector(const std::string& director) { m_director = director; }
  std::string GetDirector() const { return m_director; }

  void SetWriter(const std::string& writer) { m_writer = writer; }
  std::string GetWriter() const { return m_writer; }

  void SetYear(int year) { m_cStructure->iYear = year; }
  int GetYear() const { return m_cStructure->iYear; }

  void SetIMDBNumber(const std::string& IMDBNumber) { m_IMDBNumber = IMDBNumber; }
  std::string GetIMDBNumber() const { return m_IMDBNumber; }

  void SetIconPath(const std::string& iconPath) { m_iconPath = iconPath; }
  std::string GetIconPath() const { return m_iconPath; }

  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }
  int GetGenreType() const { return m_cStructure->iGenreType; }

  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  void SetGenreDescription(const std::string& genreDescription)
  {
    m_genreDescription = genreDescription;
  }
  std::string GetGenreDescription() const { return m_genreDescription; }

  void SetFirstAired(const std::string& firstAired) { m_firstAired = firstAired; }
  std::string GetFirstAired() const { return m_firstAired; }

  void SetParentalRating(int parentalRating) { m_cStructure->iParentalRating = parentalRating; }
  int GetParentalRating() const { return m_cStructure->iParentalRating; }

  void SetStarRating(int starRating) { m_cStructure->iStarRating = starRating; }
  int GetStarRating() const { return m_cStructure->iStarRating; }

  void SetSeriesNumber(int seriesNumber) { m_cStructure->iSeriesNumber = seriesNumber; }
  int GetSeriesNumber() const { return m_cStructure->iSeriesNumber; }

  void SetEpisodeNumber(int episodeNumber) { m_cStructure->iEpisodeNumber = episodeNumber; }
  int GetEpisodeNumber() const { return m_cStructure->iEpisodeNumber; }

  void SetEpisodePartNumber(int episodePartNumber)
  {
    m_cStructure->iEpisodePartNumber = episodePartNumber;
  }
  int GetEpisodePartNumber() const { return m_cStructure->iEpisodePartNumber; }

  void SetEpisodeName(const std::string& episodeName) { m_episodeName = episodeName; }
  std::string GetEpisodeName() const { return m_episodeName; }

  void SetFlags(unsigned int flags) { m_cStructure->iFlags = flags; }
  unsigned int GetFlags() const { return m_cStructure->iFlags; }

  void SetSeriesLink(const std::string& seriesLink) { m_seriesLink = seriesLink; }
  std::string GetSeriesLink() const { return m_seriesLink; }

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
    m_cStructure->strEpisodeName = m_episodeName.c_str();
    m_cStructure->strSeriesLink = m_seriesLink.c_str();
    m_cStructure->strFirstAired = m_firstAired.c_str();

    return m_cStructure;
  }

private:
  // prevent the use of them
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
  std::string m_firstAired;

  void SetData(const EPG_TAG* tag)
  {
    m_title = tag->strTitle;
    m_plotOutline = tag->strPlotOutline;
    m_plot = tag->strPlot;
    m_originalTitle = tag->strOriginalTitle;
    m_cast = tag->strCast;
    m_director = tag->strDirector;
    m_writer = tag->strWriter;
    m_IMDBNumber = tag->strIMDBNumber;
    m_iconPath = tag->strIconPath;
    m_genreDescription = tag->strGenreDescription;
    m_episodeName = tag->strEpisodeName;
    m_seriesLink = tag->strSeriesLink;
    m_firstAired = tag->strFirstAired;
  }
};

class PVREPGTagsResultSet
{
public:
  PVREPGTagsResultSet() = delete;
  PVREPGTagsResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVREPGTag& tag)
  {
    m_instance->toKodi->TransferEpgEntry(m_instance->toKodi->kodiInstance, m_handle, tag.GetTag());
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
