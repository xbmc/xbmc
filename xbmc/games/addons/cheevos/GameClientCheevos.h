/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h> /* size_t */
#include <string>

struct AddonInstance_Game;

namespace KODI
{
namespace RETRO
{
enum class RConsoleID;
}

namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientCheevos
{
public:
  CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  bool RCGenerateHashFromFile(std::string& hash,
                              RETRO::RConsoleID consoleID,
                              const std::string& filePath);
  bool RCGetGameIDUrl(std::string& url, const std::string& hash);
  bool RCGetPatchFileUrl(std::string& url,
                         const std::string& username,
                         const std::string& token,
                         unsigned int gameID);
  bool RCPostRichPresenceUrl(std::string& url,
                             std::string& postData,
                             const std::string& username,
                             const std::string& token,
                             unsigned gameID,
                             const std::string& richPresence);
  void RCEnableRichPresence(const std::string& script);
  void RCGetRichPresenceEvaluation(std::string& evaluation, RETRO::RConsoleID consoleID);
  // When the game is reset, the runtime should also be reset
  void RCResetRuntime();

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
