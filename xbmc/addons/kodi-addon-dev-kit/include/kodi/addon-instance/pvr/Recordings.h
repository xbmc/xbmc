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

class PVRRecording : public CStructHdl<PVRRecording, PVR_RECORDING>
{
public:
  PVRRecording()
  {
    m_cStructure->iSeriesNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    m_cStructure->iEpisodeNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
    m_cStructure->recordingTime = 0;
    m_cStructure->iDuration = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iPriority = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iLifetime = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreType = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreSubType = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iPlayCount = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->iLastPlayedPosition = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->bIsDeleted = false;
    m_cStructure->iEpgEventId = 0;
    m_cStructure->iChannelUid = PVR_RECORDING_VALUE_NOT_AVAILABLE;
    m_cStructure->channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;
    m_cStructure->iFlags = 0;
    m_cStructure->sizeInBytes = PVR_RECORDING_VALUE_NOT_AVAILABLE;
  }
  PVRRecording(const PVRRecording& recording) : CStructHdl(recording) {}
  PVRRecording(const PVR_RECORDING* recording) : CStructHdl(recording) {}
  PVRRecording(PVR_RECORDING* recording) : CStructHdl(recording) {}

  void SetRecordingId(const std::string& recordingId)
  {
    strncpy(m_cStructure->strRecordingId, recordingId.c_str(),
            sizeof(m_cStructure->strRecordingId) - 1);
  }
  std::string GetRecordingId() const { return m_cStructure->strRecordingId; }

  void SetTitle(const std::string& title)
  {
    strncpy(m_cStructure->strTitle, title.c_str(), sizeof(m_cStructure->strTitle) - 1);
  }
  std::string GetTitle() const { return m_cStructure->strTitle; }

  void SetEpisodeName(const std::string& episodeName)
  {
    strncpy(m_cStructure->strEpisodeName, episodeName.c_str(),
            sizeof(m_cStructure->strEpisodeName) - 1);
  }
  std::string GetEpisodeName() const { return m_cStructure->strEpisodeName; }

  void SetSeriesNumber(int seriesNumber) { m_cStructure->iSeriesNumber = seriesNumber; }
  int GetSeriesNumber() const { return m_cStructure->iSeriesNumber; }

  void SetEpisodeNumber(int episodeNumber) { m_cStructure->iEpisodeNumber = episodeNumber; }
  int GetEpisodeNumber() const { return m_cStructure->iEpisodeNumber; }

  void SetYear(int year) { m_cStructure->iYear = year; }
  int GetYear() const { return m_cStructure->iYear; }

  void SetDirectory(const std::string& directory)
  {
    strncpy(m_cStructure->strDirectory, directory.c_str(), sizeof(m_cStructure->strDirectory) - 1);
  }
  std::string GetDirectory() const { return m_cStructure->strDirectory; }

  void SetPlotOutline(const std::string& plotOutline)
  {
    strncpy(m_cStructure->strPlotOutline, plotOutline.c_str(),
            sizeof(m_cStructure->strPlotOutline) - 1);
  }
  std::string GetPlotOutline() const { return m_cStructure->strPlotOutline; }

  void SetPlot(const std::string& plot)
  {
    strncpy(m_cStructure->strPlot, plot.c_str(), sizeof(m_cStructure->strPlot) - 1);
  }
  std::string GetPlot() const { return m_cStructure->strPlot; }

  void SetChannelName(const std::string& channelName)
  {
    strncpy(m_cStructure->strChannelName, channelName.c_str(),
            sizeof(m_cStructure->strChannelName) - 1);
  }
  std::string GetChannelName() const { return m_cStructure->strChannelName; }

  void SetIconPath(const std::string& iconPath)
  {
    strncpy(m_cStructure->strIconPath, iconPath.c_str(), sizeof(m_cStructure->strIconPath) - 1);
  }
  std::string GetIconPath() const { return m_cStructure->strIconPath; }

  void SetThumbnailPath(const std::string& thumbnailPath)
  {
    strncpy(m_cStructure->strThumbnailPath, thumbnailPath.c_str(),
            sizeof(m_cStructure->strThumbnailPath) - 1);
  }
  std::string GetThumbnailPath() const { return m_cStructure->strThumbnailPath; }

  void SetFanartPath(const std::string& fanartPath)
  {
    strncpy(m_cStructure->strFanartPath, fanartPath.c_str(),
            sizeof(m_cStructure->strFanartPath) - 1);
  }
  std::string GetFanartPath() const { return m_cStructure->strFanartPath; }

  void SetRecordingTime(time_t recordingTime) { m_cStructure->recordingTime = recordingTime; }
  time_t GetRecordingTime() const { return m_cStructure->recordingTime; }

  void SetDuration(int duration) { m_cStructure->iDuration = duration; }
  int GetDuration() const { return m_cStructure->iDuration; }

  void SetPriority(int priority) { m_cStructure->iPriority = priority; }
  int GetPriority() const { return m_cStructure->iPriority; }

  void SetLifetime(int lifetime) { m_cStructure->iLifetime = lifetime; }
  int GetLifetime() const { return m_cStructure->iLifetime; }

  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }
  int GetGenreType() const { return m_cStructure->iGenreType; }

  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  void SetGenreDescription(const std::string& genreDescription)
  {
    strncpy(m_cStructure->strGenreDescription, genreDescription.c_str(),
            sizeof(m_cStructure->strGenreDescription) - 1);
  }
  std::string GetGenreDescription() const { return m_cStructure->strGenreDescription; }

  void SetPlayCount(int playCount) { m_cStructure->iPlayCount = playCount; }
  int GetPlayCount() const { return m_cStructure->iPlayCount; }

  void SetLastPlayedPosition(int lastPlayedPosition)
  {
    m_cStructure->iLastPlayedPosition = lastPlayedPosition;
  }
  int GetLastPlayedPosition() const { return m_cStructure->iLastPlayedPosition; }

  void SetIsDeleted(int isDeleted) { m_cStructure->bIsDeleted = isDeleted; }
  int GetIsDeleted() const { return m_cStructure->bIsDeleted; }

  void SetEPGEventId(unsigned int epgEventId) { m_cStructure->iEpgEventId = epgEventId; }
  unsigned int GetEPGEventId() const { return m_cStructure->iEpgEventId; }

  void SetChannelUid(int channelUid) { m_cStructure->iChannelUid = channelUid; }
  int GetChannelUid() const { return m_cStructure->iChannelUid; }

  void SetChannelType(PVR_RECORDING_CHANNEL_TYPE channelType)
  {
    m_cStructure->channelType = channelType;
  }
  PVR_RECORDING_CHANNEL_TYPE GetChannelType() const { return m_cStructure->channelType; }

  void SetFirstAired(const std::string& firstAired)
  {
    strncpy(m_cStructure->strFirstAired, firstAired.c_str(),
            sizeof(m_cStructure->strFirstAired) - 1);
  }
  std::string GetFirstAired() const { return m_cStructure->strFirstAired; }

  void SetFlags(unsigned int flags) { m_cStructure->iFlags = flags; }
  unsigned int GetFlags() const { return m_cStructure->iFlags; }

  void SetSizeInBytes(int64_t sizeInBytes) { m_cStructure->sizeInBytes = sizeInBytes; }
  int64_t GetSizeInBytes() const { return m_cStructure->sizeInBytes; }
};

class PVRRecordingsResultSet
{
public:
  PVRRecordingsResultSet() = delete;
  PVRRecordingsResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVRRecording& tag)
  {
    m_instance->toKodi->TransferRecordingEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
