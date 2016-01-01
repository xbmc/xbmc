#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/ISubSettings.h"
#include "settings/AudioDSPSettings.h"
#include "settings/VideoSettings.h"
#include "threads/CriticalSection.h"

#define VOLUME_DRC_MINIMUM 0    // 0dB
#define VOLUME_DRC_MAXIMUM 6000 // 60dB

class TiXmlNode;

typedef enum {
  WatchedModeAll        = 0,
  WatchedModeUnwatched,
  WatchedModeWatched
} WatchedMode;

class CMediaSettings : public ISettingCallback, public ISettingsHandler, public ISubSettings
{
public:
  static CMediaSettings& GetInstance();

  virtual bool Load(const TiXmlNode *settings) override;
  virtual bool Save(TiXmlNode *settings) const override;

  virtual void OnSettingAction(const CSetting *setting) override;
  virtual void OnSettingsLoaded() override;

  const CVideoSettings& GetDefaultVideoSettings() const { return m_defaultVideoSettings; }
  CVideoSettings& GetDefaultVideoSettings() { return m_defaultVideoSettings; }
  const CVideoSettings& GetCurrentVideoSettings() const { return m_currentVideoSettings; }
  CVideoSettings& GetCurrentVideoSettings() { return m_currentVideoSettings; }

  const CAudioSettings& GetDefaultAudioSettings() const { return m_defaultAudioSettings; }
  CAudioSettings& GetDefaultAudioSettings() { return m_defaultAudioSettings; }
  const CAudioSettings& GetCurrentAudioSettings() const { return m_currentAudioSettings; }
  CAudioSettings& GetCurrentAudioSettings() { return m_currentAudioSettings; }

  /*! \brief Retreive the watched mode for the given content type
   \param content Current content type
   \return the current watch mode for this content type, WATCH_MODE_ALL if the content type is unknown.
   \sa SetWatchMode
   */
  int GetWatchedMode(const std::string &content) const;

  /*! \brief Set the watched mode for the given content type
   \param content Current content type
   \param value Watched mode to set
   \sa GetWatchMode
   */
  void SetWatchedMode(const std::string &content, WatchedMode mode);

  /*! \brief Cycle the watched mode for the given content type
   \param content Current content type
   \sa GetWatchMode, SetWatchMode
   */
  void CycleWatchedMode(const std::string &content);

  void SetMusicPlaylistRepeat(bool repeats) { m_musicPlaylistRepeat = repeats; }
  void SetMusicPlaylistShuffled(bool shuffled) { m_musicPlaylistShuffle = shuffled; }

  void SetVideoPlaylistRepeat(bool repeats) { m_videoPlaylistRepeat = repeats; }
  void SetVideoPlaylistShuffled(bool shuffled) { m_videoPlaylistShuffle = shuffled; }

  bool DoesVideoStartWindowed() const { return m_videoStartWindowed; }
  void SetVideoStartWindowed(bool windowed) { m_videoStartWindowed = windowed; }
  int GetAdditionalSubtitleDirectoryChecked() const { return m_additionalSubtitleDirectoryChecked; }
  void SetAdditionalSubtitleDirectoryChecked(int checked) { m_additionalSubtitleDirectoryChecked = checked; }

  int GetMusicNeedsUpdate() const { return m_musicNeedsUpdate; }
  void SetMusicNeedsUpdate(int version) { m_musicNeedsUpdate = version; }
  int GetVideoNeedsUpdate() const { return m_videoNeedsUpdate; }
  void SetVideoNeedsUpdate(int version) { m_videoNeedsUpdate = version; }

protected:
  CMediaSettings();
  CMediaSettings(const CMediaSettings&);
  CMediaSettings& operator=(CMediaSettings const&);
  virtual ~CMediaSettings();

  static std::string GetWatchedContent(const std::string &content);

private:
  CVideoSettings m_defaultVideoSettings;
  CVideoSettings m_currentVideoSettings;

  CAudioSettings m_defaultAudioSettings;
  CAudioSettings m_currentAudioSettings;

  typedef std::map<std::string, WatchedMode> WatchedModes;
  WatchedModes m_watchedModes;

  bool m_musicPlaylistRepeat;
  bool m_musicPlaylistShuffle;
  bool m_videoPlaylistRepeat;
  bool m_videoPlaylistShuffle;

  bool m_videoStartWindowed;
  int m_additionalSubtitleDirectoryChecked;

  int m_musicNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)
  int m_videoNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)

  CCriticalSection m_critical;
};
