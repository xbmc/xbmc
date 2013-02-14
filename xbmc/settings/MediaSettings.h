#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include "settings/ISubSettings.h"
#include "settings/VideoSettings.h"

#define VOLUME_DRC_MINIMUM         0 // 0dB
#define VOLUME_DRC_MAXIMUM      6000 // 60dB

enum ViewMode
{
  VIEW_MODE_NORMAL        = 0,
  VIEW_MODE_ZOOM,
  VIEW_MODE_STRETCH_4x3,
  VIEW_MODE_WIDE_ZOOM,
  VIEW_MODE_STRETCH_16x9,
  VIEW_MODE_ORIGINAL,
  VIEW_MODE_CUSTOM
};

typedef enum {
  WatchedModeAll        = 0,
  WatchedModeUnwatched,
  WatchedModeWatched
} WatchedMode;

class TiXmlNode;

class CMediaSettings : public ISubSettings
{
public:
  static CMediaSettings& Get();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  const std::string& GetPictureExtensions() const { return m_pictureExtensions; }
  void SetPictureExtensions(const std::string &extensions) { m_pictureExtensions = extensions; }
  const std::string& GetMusicExtensions() const { return m_musicExtensions; }
  void SetMusicExtensions(const std::string &extensions) { m_musicExtensions = extensions; }
  const std::string& GetVideoExtensions() const { return m_videoExtensions; }
  void SetVideoExtensions(const std::string &extensions) { m_videoExtensions = extensions; }
  const std::string& GetDiscStubExtensions() const { return m_discStubExtensions; }
  void SetDiscStubExtensions(const std::string &extensions) { m_discStubExtensions = extensions; }

  bool ShowSongThumbInVisualisation() const { return m_bMyMusicSongThumbInVis; }
  void SetSongThumbInVisualisation(bool show) { m_bMyMusicSongThumbInVis = show; }
  bool DoesMusicPlaylistRepeat() const { return m_bMyMusicPlaylistRepeat; }
  void SetMusicPlaylistRepeat(bool repeat) { m_bMyMusicPlaylistRepeat = repeat; }
  bool IsMusicPlaylistShuffled() const { return m_bMyMusicPlaylistShuffle; }
  void SetMusicPlaylistShuffled(bool shuffled) { m_bMyMusicPlaylistShuffle = shuffled; }
  int GetMusicStartWindow() const { return m_iMyMusicStartWindow; }
  void SetMusicStartWindow(int startWindow) { m_iMyMusicStartWindow = startWindow; }
  const std::string& GetDefaultMusicLibrarySource() const { return m_defaultMusicLibSource; }
  void SetDefaultMusicLibrarySource(const std::string& source) { m_defaultMusicLibSource = source; }

  bool DoesVideoPlaylistRepeat() const { return m_bMyVideoPlaylistRepeat; }
  void SetVideoPlaylistRepeat(bool repeat) { m_bMyVideoPlaylistRepeat = repeat; }
  bool IsVideoPlaylistShuffled() const { return m_bMyVideoPlaylistShuffle; }
  void SetVideoPlaylistShuffled(bool shuffled) { m_bMyVideoPlaylistShuffle = shuffled; }
  bool IsVideoNavigationFlattened() const { return m_bMyVideoNavFlatten; }
  void SetVideoNavigationFlattened(bool flatten) { m_bMyVideoNavFlatten = flatten; }
  bool DoesVideoStartWindowed() const { return m_bStartVideoWindowed; }
  void SetVideoStartWindowed(bool startWindowed) { m_bStartVideoWindowed = startWindowed; }
  bool AreVideosStacked() const { return m_videoStacking; }
  void SetVideosStacked(bool stacked) { m_videoStacking = stacked; }
  int GetVideoStartWindow() const { return m_iVideoStartWindow; }
  void SetVideoStartWindow(int startWindow) { m_iVideoStartWindow = startWindow; }

  const CVideoSettings& GetDefaultVideoSettings() const { return m_defaultVideoSettings; }
  CVideoSettings& GetDefaultVideoSettings() { return m_defaultVideoSettings; }
  const CVideoSettings& GetCurrentVideoSettings() const { return m_currentVideoSettings; }
  CVideoSettings& GetCurrentVideoSettings() { return m_currentVideoSettings; }

  int GetAdditionalSubtitleDirectoryCheckedState() const { return iAdditionalSubtitleDirectoryChecked; }
  void SetAdditionalSubtitleDirectoryCheckedState(int checked) { iAdditionalSubtitleDirectoryChecked = checked; }

  int GetMusicDatabaseUpdateVersion() const { return m_musicNeedsUpdate; }
  void SetMusicDatabaseUpdateVersion(int version) { m_musicNeedsUpdate = version; }
  int GetVideoDatabaseUpdateVersion() const { return m_videoNeedsUpdate; }
  void SetVideoDatabaseUpdateVersion(int version) { m_videoNeedsUpdate = version; }

  /*! \brief Retreive the watched mode for the given content type
    \param content Current content type
    \return the current watch mode for this content type, WatchedModeAll if the content type is unknown.
    \sa SetWatchMode, IncrementWatchMode
    */
  WatchedMode GetWatchedMode(const std::string& content) const;

  /*! \brief Set the watched mode for the given content type
    \param content Current content type
    \param value Watched mode to set
    \sa GetWatchMode, IncrementWatchMode
    */
  void SetWatchedMode(const std::string& content, WatchedMode value);

  /*! \brief Cycle the watched mode for the given content type
    \param content Current content type
    \sa GetWatchMode, SetWatchMode
    */
  void CycleWatchedMode(const std::string& content);

protected:
  CMediaSettings();
  CMediaSettings(const CMediaSettings&);
  CMediaSettings const& operator=(CMediaSettings const&);
  virtual ~CMediaSettings();

  static std::string GetWatchedContent(const std::string &content);

private:
  std::string m_pictureExtensions;
  std::string m_musicExtensions;
  std::string m_videoExtensions;
  std::string m_discStubExtensions;

  bool m_bMyMusicSongThumbInVis;
  bool m_bMyMusicPlaylistRepeat;
  bool m_bMyMusicPlaylistShuffle;
  int m_iMyMusicStartWindow;
  std::string m_defaultMusicLibSource;

  bool m_bMyVideoPlaylistRepeat;
  bool m_bMyVideoPlaylistShuffle;
  bool m_bMyVideoNavFlatten;
  bool m_bStartVideoWindowed;
  bool m_videoStacking;
  int m_iVideoStartWindow;

  CVideoSettings m_defaultVideoSettings;
  CVideoSettings m_currentVideoSettings;

  int iAdditionalSubtitleDirectoryChecked;

  int m_musicNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)
  int m_videoNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)
  
  std::map<std::string, WatchedMode> m_watchedModes;
};