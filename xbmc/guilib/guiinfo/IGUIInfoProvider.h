/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
   * @brief Get a GUIInfoManager label fallback string. Will be called if none of the registered
   * provider's GetLabel() implementation has returned success.
   * @param value Will be filled with the requested value.
   * @param item The item to get the value for. Can be nullptr.
   * @param contextWindow The context window. Can be 0.
   * @param info The GUI info (label id + additional data).
   * @param fallback A fallback value. Can be nullptr.
   * @return True if the value was filled successfully, false otherwise.
   */
  virtual bool GetFallbackLabel(std::string& value,
                                const CFileItem* item,
                                int contextWindow,
                                const CGUIInfo& info,
                                std::string* fallback) = 0;

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
  virtual void UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo, const SubtitleStreamInfo& subtitleInfo) = 0;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
