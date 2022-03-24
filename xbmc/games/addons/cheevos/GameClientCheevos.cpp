/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheevos.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "cores/RetroPlayer/cheevos/RConsoleIDs.h"
#include "games/addons/GameClient.h"

using namespace KODI;
using namespace GAME;

CGameClientCheevos::CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient), m_struct(addonStruct)
{
}

bool CGameClientCheevos::RCGenerateHashFromFile(std::string& hash,
                                                RETRO::RConsoleID consoleID,
                                                const std::string& filePath)
{
  char* _hash = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->RCGenerateHashFromFile(
            &m_struct, &_hash, static_cast<unsigned int>(consoleID), filePath.c_str()),
        "RCGenerateHashFromFile()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetGameIDUrl()");
  }

  if (_hash)
  {
    hash = _hash;
    free(_hash);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetGameIDUrl(std::string& url, const std::string& hash)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetGameIDUrl(&m_struct, &_url, hash.c_str()),
                          "RCGetGameIDUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetGameIDUrl()");
  }

  if (_url)
  {
    url = _url;
    free(_url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCGetPatchFileUrl(std::string& url,
                                           const std::string& username,
                                           const std::string& token,
                                           unsigned int gameID)
{
  char* _url = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetPatchFileUrl(
                              &m_struct, &_url, username.c_str(), token.c_str(), gameID),
                          "RCGetPatchFileUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetPatchFileUrl()");
  }

  if (_url)
  {
    url = _url;
    free(_url);
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientCheevos::RCPostRichPresenceUrl(std::string& url,
                                               std::string& postData,
                                               const std::string& username,
                                               const std::string& token,
                                               unsigned gameID,
                                               const std::string& richPresence)
{
  char* _url = nullptr;
  char* _postData = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCPostRichPresenceUrl(
                              &m_struct, &_url, &_postData, username.c_str(), token.c_str(), gameID,
                              richPresence.c_str()),
                          "RCPostRichPresenceUrl()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCPostRichPresenceUrl()");
  }

  if (_url)
  {
    url = _url;
    free(_url);
  }
  if (_postData)
  {
    postData = _postData;
    free(_postData);
  }

  return error == GAME_ERROR_NO_ERROR;
}

void CGameClientCheevos::RCEnableRichPresence(const std::string& script)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCEnableRichPresence(&m_struct, script.c_str()),
                          "RCEnableRichPresence()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCEnableRichPresence()");
  }
}

void CGameClientCheevos::RCGetRichPresenceEvaluation(std::string& evaluation,
                                                     RETRO::RConsoleID consoleID)
{
  char* _evaluation = nullptr;
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCGetRichPresenceEvaluation(
                              &m_struct, &_evaluation, static_cast<unsigned int>(consoleID)),
                          "RCGetRichPresenceEvaluation()");

    if (_evaluation)
    {
      evaluation = _evaluation;
      free(_evaluation);
    }
  }
  catch (...)
  {
    m_gameClient.LogException("RCGetRichPresenceEvaluation()");
  }
}

void CGameClientCheevos::RCResetRuntime()
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RCResetRuntime(&m_struct), "RCResetRuntime()");
  }
  catch (...)
  {
    m_gameClient.LogException("RCResetRuntime()");
  }
}
