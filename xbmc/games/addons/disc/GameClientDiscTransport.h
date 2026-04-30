/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct AddonInstance_Game;
class CCriticalSection;

namespace KODI
{
namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientDiscTransport
{
public:
  CGameClientDiscTransport(CGameClient& gameClient,
                           AddonInstance_Game& addonStruct,
                           CCriticalSection& clientAccess);

  // Game client functions
  bool GetEjectState();
  bool SetEjectState(bool ejected);
  unsigned int GetImageIndex();
  bool SetImageIndex(unsigned int imageIndex);
  unsigned int GetImageCount();
  bool AddImageIndex();
  bool ReplaceImageIndex(unsigned int imageIndex, const std::string& filePath);
  bool RemoveImageIndex(unsigned int imageIndex);
  bool SetInitialImage(unsigned int imageIndex, const std::string& filePath);
  std::string GetImagePath(unsigned int imageIndex);
  std::string GetImageLabel(unsigned int imageIndex);

private:
  // Construction parameters
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
  CCriticalSection& m_clientAccess;
};
} // namespace GAME
} // namespace KODI
