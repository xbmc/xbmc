/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO
{
class CGUIGameRenderManager;
class IPlayback;

class CGUIGameSettingsHandle
{
public:
  CGUIGameSettingsHandle(CGUIGameRenderManager& renderManager);
  virtual ~CGUIGameSettingsHandle();

  /*!
   * \brief Get the ID of the active game client
   *
   * \return The ID of the active game client, or empty string if a game is
   * not playing
   */
  std::string GameClientID();
  std::string GetPlayingGame();
  std::string CreateSavestate(bool autosave);
  bool LoadSavestate(const std::string& path);
  void CloseOSD();

private:
  // Construction parameters
  CGUIGameRenderManager& m_renderManager;
};
} // namespace RETRO
} // namespace KODI
