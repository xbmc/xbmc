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

class IGUIInfoProvider
{
public:
  virtual ~IGUIInfoProvider() = default;

  /*!
   * @brief Init a new current guiinfo manager item. Gets called whenever the active guiinfo manager item changes.
   * @param item The new item.
   * @return True if the item was inited by the provider, false otherwise.
   */
  virtual bool InitCurrentItem(CFileItem *item) = 0;

  /*!
   * @brief Get a GUIInfoManager label string.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @param fallback A fallback value. Can be nullptr.
   * @return True if the value was filled successfully, false otherwise.
   */
  virtual bool GetLabel(std::string &value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const = 0;

  /*!
   * @brief Get a GUIInfoManager integer value.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully, false otherwise.
   */
  virtual bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const = 0;

  /*!
   * @brief Get a GUIInfoManager bool value.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @return True if the value was filled successfully, false otherwise.
   */
  virtual bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const = 0;

  /*!
   * @brief Set new audio/video stream info data.
   * @param audioInfo New audio stream info.
   * @param videoInfo New video stream info.
   */
  virtual void UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo) = 0;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
