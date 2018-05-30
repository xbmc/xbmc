/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>
#include <vector>

#include "guilib/guiinfo/AddonsGUIInfo.h"
#include "guilib/guiinfo/GamesGUIInfo.h"
#include "guilib/guiinfo/GUIControlsGUIInfo.h"
#include "guilib/guiinfo/LibraryGUIInfo.h"
#include "guilib/guiinfo/MusicGUIInfo.h"
#include "guilib/guiinfo/PicturesGUIInfo.h"
#include "guilib/guiinfo/PlayerGUIInfo.h"
#include "guilib/guiinfo/SkinGUIInfo.h"
#include "guilib/guiinfo/SystemGUIInfo.h"
#include "guilib/guiinfo/VideoGUIInfo.h"
#include "guilib/guiinfo/VisualisationGUIInfo.h"
#include "guilib/guiinfo/WeatherGUIInfo.h"

class CFileItem;
class CGUIListItem;

struct AudioStreamInfo;
struct VideoStreamInfo;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;
class IGUIInfoProvider;

class CGUIInfoProviders
{
public:
  CGUIInfoProviders();
  virtual ~CGUIInfoProviders();

  /*!
   * @brief Register a guiinfo provider.
   * @param provider The provider to register.
   * @param bAppend True to append to the list of providers, false to insert before the first provider
   */
  void RegisterProvider(IGUIInfoProvider *provider, bool bAppend = true);

  /*!
   * @brief Unregister a guiinfo provider.
   * @param provider The provider to unregister.
   */
  void UnregisterProvider(IGUIInfoProvider *provider);

  /*!
   * @brief Init a new current guiinfo manager item. Gets called whenever the active guiinfo manager item changes.
   * @param item The new item.
   * @return True if the item was inited by one of the providers, false otherwise.
   */
  bool InitCurrentItem(CFileItem *item);

  /*!
   * @brief Get a GUIInfoManager label string from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @param fallback A fallback value. Can be nullptr.
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const;

  /*!
   * @brief Get a GUIInfoManager integer value from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

  /*!
   * @brief Get a GUIInfoManager bool value from one of the registered providers.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully by one of the providers, false otherwise.
   */
  bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

  /*!
   * @brief Set new audio/video stream info data at all registered providers.
   * @param audioInfo New audio stream info.
   * @param videoInfo New video stream info.
   */
  void UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo);

  /*!
   * @brief Get the player guiinfo provider.
   * @return The player guiinfo provider.
   */
  CPlayerGUIInfo& GetPlayerInfoProvider() { return m_playerGUIInfo; }

  /*!
   * @brief Get the system guiinfo provider.
   * @return The system guiinfo provider.
   */
  CSystemGUIInfo& GetSystemInfoProvider() { return m_systemGUIInfo; }

  /*!
   * @brief Get the pictures guiinfo provider.
   * @return The pictures guiinfo provider.
   */
  CPicturesGUIInfo& GetPicturesInfoProvider() { return m_picturesGUIInfo; }

  /*!
   * @brief Get the gui controls guiinfo provider.
   * @return The gui controls guiinfo provider.
   */
  CGUIControlsGUIInfo& GetGUIControlsInfoProvider() { return m_guiControlsGUIInfo; }

  /*!
   * @brief Get the library guiinfo provider.
   * @return The library guiinfo provider.
   */
  CLibraryGUIInfo& GetLibraryInfoProvider() { return m_libraryGUIInfo; }

private:
  std::vector<IGUIInfoProvider *> m_providers;

  CAddonsGUIInfo m_addonsGUIInfo;
  CGamesGUIInfo m_gamesGUIInfo;
  CGUIControlsGUIInfo m_guiControlsGUIInfo;
  CLibraryGUIInfo m_libraryGUIInfo;
  CMusicGUIInfo m_musicGUIInfo;
  CPicturesGUIInfo m_picturesGUIInfo;
  CPlayerGUIInfo m_playerGUIInfo;
  CSkinGUIInfo m_skinGUIInfo;
  CSystemGUIInfo m_systemGUIInfo;
  CVideoGUIInfo m_videoGUIInfo;
  CVisualisationGUIInfo m_visualisationGUIInfo;
  CWeatherGUIInfo m_weatherGUIInfo;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
