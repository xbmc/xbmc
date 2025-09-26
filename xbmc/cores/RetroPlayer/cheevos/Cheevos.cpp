/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Cheevos.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/tags/GameInfoTag.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/JSONVariantParser.h"
#include "utils/Map.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <string_view>
#include <vector>

using namespace KODI;
using namespace RETRO;

namespace
{
// API JSON Field names
constexpr auto SUCCESS = "Success";
constexpr auto GAME_ID = "GameID";
constexpr auto PATCH_DATA = "PatchData";
constexpr auto RICH_PRESENCE = "RichPresencePatch";
constexpr auto GAME_TITLE = "Title";
constexpr auto PUBLISHER = "Publisher";
constexpr auto DEVELOPER = "Developer";
constexpr auto GENRE = "Genre";
constexpr auto CONSOLE_NAME = "ConsoleName";

constexpr int RESPONSE_SIZE = 64;

constexpr auto extensionToConsole = make_map<std::string_view, RConsoleID>({
    {".a26", RConsoleID::RC_CONSOLE_ATARI_2600},
    {".a78", RConsoleID::RC_CONSOLE_ATARI_7800},
    {".agb", RConsoleID::RC_CONSOLE_GAMEBOY_ADVANCE},
    {".cdi", RConsoleID::RC_CONSOLE_DREAMCAST},
    {".cdt", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
    {".cgb", RConsoleID::RC_CONSOLE_GAMEBOY_COLOR},
    {".chd", RConsoleID::RC_CONSOLE_DREAMCAST},
    {".cpr", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
    {".d64", RConsoleID::RC_CONSOLE_COMMODORE_64},
    {".gb", RConsoleID::RC_CONSOLE_GAMEBOY},
    {".gba", RConsoleID::RC_CONSOLE_GAMEBOY_ADVANCE},
    {".gbc", RConsoleID::RC_CONSOLE_GAMEBOY_COLOR},
    {".gdi", RConsoleID::RC_CONSOLE_DREAMCAST},
    {".j64", RConsoleID::RC_CONSOLE_ATARI_JAGUAR},
    {".jag", RConsoleID::RC_CONSOLE_ATARI_JAGUAR},
    {".lnx", RConsoleID::RC_CONSOLE_ATARI_LYNX},
    {".mds", RConsoleID::RC_CONSOLE_SATURN},
    {".min", RConsoleID::RC_CONSOLE_POKEMON_MINI},
    {".mx1", RConsoleID::RC_CONSOLE_MSX},
    {".mx2", RConsoleID::RC_CONSOLE_MSX},
    {".n64", RConsoleID::RC_CONSOLE_NINTENDO_64},
    {".ndd", RConsoleID::RC_CONSOLE_NINTENDO_64},
    {".nds", RConsoleID::RC_CONSOLE_NINTENDO_DS},
    {".nes", RConsoleID::RC_CONSOLE_NINTENDO},
    {".o", RConsoleID::RC_CONSOLE_ATARI_LYNX},
    {".pce", RConsoleID::RC_CONSOLE_PC_ENGINE},
    {".sfc", RConsoleID::RC_CONSOLE_SUPER_NINTENDO},
    {".sgx", RConsoleID::RC_CONSOLE_PC_ENGINE},
    {".smc", RConsoleID::RC_CONSOLE_SUPER_NINTENDO},
    {".sna", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
    {".tap", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
    {".u1", RConsoleID::RC_CONSOLE_NINTENDO_64},
    {".v64", RConsoleID::RC_CONSOLE_NINTENDO_64},
    {".vb", RConsoleID::RC_CONSOLE_VIRTUAL_BOY},
    {".vboy", RConsoleID::RC_CONSOLE_VIRTUAL_BOY},
    {".vec", RConsoleID::RC_CONSOLE_VECTREX},
    {".voc", RConsoleID::RC_CONSOLE_AMSTRAD_PC},
    {".z64", RConsoleID::RC_CONSOLE_NINTENDO_64},
});
} // namespace

CCheevos::CCheevos(GAME::CGameClient* gameClient,
                   const std::string& userName,
                   const std::string& loginToken)
  : m_gameClient(gameClient),
    m_userName(userName),
    m_loginToken(loginToken)
{
}

void CCheevos::ResetRuntime()
{
  m_gameClient->Cheevos().RCResetRuntime();
}

bool CCheevos::LoadData()
{
  if (m_userName.empty() || m_loginToken.empty())
    return false;

  if (m_romHash.empty())
  {
    m_consoleID = ConsoleID();
    if (m_consoleID == RConsoleID::RC_INVALID_ID)
      return false;

    std::string hash;
    if (!m_gameClient->Cheevos().RCGenerateHashFromFile(hash, m_consoleID,
                                                        m_gameClient->GetGamePath().c_str()))
    {
      return false;
    }

    m_romHash = hash;
  }

  std::string requestURL;

  if (!m_gameClient->Cheevos().RCGetGameIDUrl(requestURL, m_romHash))
    return false;

  XFILE::CFile response;
  response.CURLCreate(requestURL);
  response.CURLOpen(0);

  char responseStr[RESPONSE_SIZE];
  response.ReadLine(responseStr, RESPONSE_SIZE);

  response.Close();

  CVariant data(CVariant::VariantTypeObject);
  CJSONVariantParser::Parse(responseStr, data);

  if (!data[SUCCESS].asBoolean())
    return false;

  m_gameID = data[GAME_ID].asUnsignedInteger32();

  // For some reason RetroAchievements returns Success = true when the hash isn't found
  if (m_gameID == 0)
    return false;

  if (!m_gameClient->Cheevos().RCGetPatchFileUrl(requestURL, m_userName, m_loginToken, m_gameID))
    return false;

  CURL curl(requestURL);
  std::vector<uint8_t> patchData;
  response.LoadFile(curl, patchData);

  std::string strResponse(patchData.begin(), patchData.end());
  CJSONVariantParser::Parse(strResponse, data);

  if (!data[SUCCESS].asBoolean())
    return false;

  m_richPresenceScript = data[PATCH_DATA][RICH_PRESENCE].asString();
  m_richPresenceLoaded = true;

  std::unique_ptr<CFileItem> file{std::make_unique<CFileItem>()};

  GAME::CGameInfoTag& tag = *file->GetGameInfoTag();
  tag.SetTitle(data[PATCH_DATA][GAME_TITLE].asString());
  tag.SetPublisher(data[PATCH_DATA][PUBLISHER].asString());
  tag.SetDeveloper(data[PATCH_DATA][DEVELOPER].asString());
  tag.SetGenres({data[PATCH_DATA][GENRE].asString()});
  tag.SetPlatform(data[PATCH_DATA][CONSOLE_NAME].asString());

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_UPDATE_PLAYER_ITEM, -1, -1,
                                             static_cast<void*>(file.release()));

  return true;
}

void CCheevos::EnableRichPresence()
{
  if (!m_richPresenceLoaded)
  {
    if (!LoadData())
    {
      CLog::Log(LOGERROR, "Cheevos: Couldn't load patch file");
      return;
    }
  }

  m_gameClient->Cheevos().RCEnableRichPresence(m_richPresenceScript);
  m_richPresenceScript.clear();
}

std::string CCheevos::GetRichPresenceEvaluation()
{
  if (!m_richPresenceLoaded)
  {
    CLog::Log(LOGERROR, "Cheevos: Rich Presence script was not found");
    return "";
  }

  std::string evaluation;
  m_gameClient->Cheevos().RCGetRichPresenceEvaluation(evaluation, m_consoleID);

  std::string url;
  std::string postData;
  if (m_gameClient->Cheevos().RCPostRichPresenceUrl(url, postData, m_userName, m_loginToken,
                                                    m_gameID, evaluation))
  {
    XFILE::CCurlFile curl;
    std::string res;
    curl.Post(url, postData, res);
  }

  return evaluation;
}

RConsoleID CCheevos::ConsoleID()
{
  const std::string extension = URIUtils::GetExtension(m_gameClient->GetGamePath());
  return extensionToConsole.get(extension).value_or(RConsoleID::RC_INVALID_ID);
}
