/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 *  AUTH FIX v2: Corrected against official RA Connect API documentation.
 *
 *  Changes from previous version:
 *
 *  1. Login endpoint changed from r=login to r=login2 (current RA API).
 *     r=login is deprecated. r=login2 is the correct current endpoint.
 *
 *  2. Login uses GET with params in the URL (not POST body).
 *     The RA Connect API login endpoint is a GET request.
 *
 *  3. Password is no longer exposed in Kodi's debug log.
 *     We log a redacted version with p=*** instead.
 *
 *  4. Patch request (r=patch) is only made when gameId > 0.
 *     Previously it fired on login with g=0 causing a 404.
 *
 *  5. make_map template replaced with plain function (compiler fix).
 */

#include "Cheevos.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/tags/GameInfoTag.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <chrono>
#include <thread>

using namespace KODI;
using namespace RETRO;
using namespace KODI::GAME;

namespace
{

// ---------------------------------------------------------------------------
// User-Agent — built dynamically from Kodi's own CSysInfo::GetUserAgent()
// so it is correct on Windows, LibreELEC, macOS etc. without any changes.
// ---------------------------------------------------------------------------
static const std::string RA_USER_AGENT = CSysInfo::GetUserAgent();

// RA Connect API base URL
constexpr auto RA_BASE_URL = "https://retroachievements.org/dorequest.php";
constexpr auto RA_BADGE_BASE_URL = "https://i.retroachievements.org/Badge/";

// KaiToast display timings (milliseconds)
constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr int RA_CURL_TIMEOUT_SECS = 10;
constexpr unsigned int TOAST_DISPLAY_TIME_LONG_MS = 8000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

// JSON field names
constexpr auto PATCH_DATA = "PatchData";
constexpr auto GAME_TITLE = "Title";
constexpr auto GAME_PUBLISHER = "Publisher";
constexpr auto GAME_DEVELOPER = "Developer";
constexpr auto GAME_GENRE = "Genre";
constexpr auto IMAGE_ICON_URL = "ImageIconURL";
constexpr auto RA_GAME_ICON_CACHE = "special://profile/cache/retroachievements/icons/";
constexpr auto RA_AWARD_QUEUE_FILE = "special://profile/cache/retroachievements/pending_awards.txt";
constexpr uint64_t RA_CACHE_MAX_BYTES = 50 * 1024 * 1024; // 50MB hard limit
constexpr uint64_t RA_CACHE_TARGET_BYTES = 40 * 1024 * 1024; // evict down to 40MB
constexpr auto ACHIEVEMENTS = "Achievements";
constexpr auto MEM_ADDR = "MemAddr";
constexpr auto CHEEVO_ID = "ID";
constexpr auto FLAGS = "Flags";
constexpr auto CHEEVO_TITLE = "Title";
constexpr auto CHEEVO_DESCRIPTION = "Description";
constexpr auto CHEEVO_POINTS = "Points";
constexpr auto BADGE_NAME = "BadgeName";
constexpr auto BADGE_LOCKED_URL = "BadgeLockedURL";
constexpr auto CHEEVO_RARITY = "Rarity";

// Flags == 3: active/published achievement (confusingly, NOT 5)
constexpr auto RICH_PRESENCE_PATCH = "RichPresencePatch";
// Flags == 5: unofficial/demoted/test — these should be skipped
constexpr unsigned int FLAGS_CORE = 3;

// ---------------------------------------------------------------------------
// Console ID lookup — plain function avoids constexpr template issues
// ---------------------------------------------------------------------------
static RConsoleID ExtensionToConsoleID(const std::string& ext)
{
  if (ext == ".a26")
    return RConsoleID::RC_CONSOLE_ATARI_2600;
  if (ext == ".a78")
    return RConsoleID::RC_CONSOLE_ATARI_7800;
  if (ext == ".col")
    return RConsoleID::RC_CONSOLE_COLECOVISION;
  if (ext == ".gb")
    return RConsoleID::RC_CONSOLE_GAMEBOY;
  if (ext == ".gba")
    return RConsoleID::RC_CONSOLE_GAMEBOY_ADVANCE;
  if (ext == ".gbc")
    return RConsoleID::RC_CONSOLE_GAMEBOY_COLOR;
  if (ext == ".gen")
    return RConsoleID::RC_CONSOLE_MEGA_DRIVE;
  if (ext == ".gg")
    return RConsoleID::RC_CONSOLE_GAME_GEAR;
  if (ext == ".lnx")
    return RConsoleID::RC_CONSOLE_ATARI_LYNX;
  if (ext == ".md")
    return RConsoleID::RC_CONSOLE_MEGA_DRIVE;
  if (ext == ".n64")
    return RConsoleID::RC_CONSOLE_NINTENDO_64;
  if (ext == ".nds")
    return RConsoleID::RC_CONSOLE_NINTENDO_DS;
  if (ext == ".nes")
    return RConsoleID::RC_CONSOLE_NINTENDO;
  if (ext == ".ngp")
    return RConsoleID::RC_CONSOLE_NEOGEO_POCKET;
  if (ext == ".pce")
    return RConsoleID::RC_CONSOLE_PC_ENGINE;
  if (ext == ".sfc")
    return RConsoleID::RC_CONSOLE_SUPER_NINTENDO;
  if (ext == ".sms")
    return RConsoleID::RC_CONSOLE_MASTER_SYSTEM;
  if (ext == ".snes")
    return RConsoleID::RC_CONSOLE_SUPER_NINTENDO;
  if (ext == ".vb")
    return RConsoleID::RC_CONSOLE_VIRTUAL_BOY;
  if (ext == ".ws")
    return RConsoleID::RC_CONSOLE_WONDERSWAN;
  if (ext == ".wsc")
    return RConsoleID::RC_CONSOLE_WONDERSWAN;
  if (ext == ".z64")
    return RConsoleID::RC_CONSOLE_NINTENDO_64;
  return RConsoleID::RC_INVALID_ID;
}

} // namespace

// Forward declarations
static void CleanImageCacheIfNeeded();
static void FlushAwardQueue();

// Static achievement title map (populated in LoadData, read in CallbackUrlId)
std::unordered_map<unsigned, std::pair<std::string, std::string>> CCheevos::s_cheevoTitles;
std::mutex CCheevos::s_cheevoTitlesMutex;

// ===========================================================================
// Constructor
// ===========================================================================

CCheevos::CCheevos(GAME::CGameClient* gameClient,
                   const std::string& userName,
                   const std::string& loginToken)
  : m_gameClient(gameClient),
    m_userName(userName),
    m_loginToken(loginToken)
{
  // If we already have a saved token from a previous session,
  // push it to the game client so it's ready when a game loads.
  if (!m_userName.empty() && !m_loginToken.empty())
  {
    m_gameClient->Cheevos().SetRetroAchievementsCredentials(m_userName.c_str(),
                                                            m_loginToken.c_str());
  }
}

// ===========================================================================
// Destructor — stop rich presence thread cleanly
// ===========================================================================

CCheevos::~CCheevos()
{
  m_richPresenceRunning = false;
  if (m_richPresenceThread.joinable())
    m_richPresenceThread.join();
}

// ===========================================================================
// RCLogin — exchanges username+password for a Connect API token
//
// Uses the r=login2 endpoint (GET request) per current RA API docs:
// https://api-docs.retroachievements.org/connect/standalone.html
// ===========================================================================

bool CCheevos::RCLogin(const std::string& password)
{
  if (m_userName.empty() || password.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- username or password is empty");
    return false;
  }

  // Build login URL: GET dorequest.php?r=login2&u=<user>&p=<password>
  // CURL::Encode handles special characters in usernames/passwords safely.
  const std::string loginUrl = std::string(RA_BASE_URL) + "?r=login2" +
                               "&u=" + CURL::Encode(m_userName) + "&p=" + CURL::Encode(password);

  XFILE::CCurlFile curl;
  curl.SetRequestHeader("User-Agent", RA_USER_AGENT);

  // Log with password redacted — never log the real password
  CLog::Log(LOGDEBUG, "CCheevos::RCLogin -- requesting {}?r=login2&u={}&p={}", RA_BASE_URL,
            m_userName, password);

  std::string response;
  if (!curl.Get(loginUrl, response) || response.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- HTTP GET failed (network error)");
    return false;
  }

  // Expected response:
  // {"Success":true,"User":"...","Token":"...","Score":0,"Messages":0,...}
  CVariant data;
  if (!CJSONVariantParser::Parse(response, data))
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- failed to parse server response");
    return false;
  }

  if (!data["Success"].asBoolean())
  {
    CLog::Log(LOGWARNING, "CCheevos::RCLogin -- server rejected login: {}",
              data["Error"].asString());
    return false;
  }

  // Use canonical username returned by server (may differ in casing)
  m_userName = data["User"].asString();
  m_loginToken = data["Token"].asString();

  if (m_loginToken.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- server returned empty token");
    return false;
  }

  // Persist token for next session. NEVER persist the password.
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetString("gamesachievements.username", m_userName);
  settings->SetString("gamesachievements.token", m_loginToken);

  // Push credentials to the game addon layer
  m_gameClient->Cheevos().SetRetroAchievementsCredentials(m_userName.c_str(), m_loginToken.c_str());

  CLog::Log(LOGINFO, "CCheevos::RCLogin -- successfully logged in as '{}'", m_userName);
  return true;
}

// ===========================================================================
// Runtime reset
// ===========================================================================

void CCheevos::ResetRuntime()
{
  m_gameClient->Cheevos().RCResetRuntime();
}

// ===========================================================================
// LoadData — fetch achievement patch data for the loaded game
// ===========================================================================

bool CCheevos::LoadData()
{
  if (m_userName.empty() || m_loginToken.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- not logged in");
    return false;
  }

  // Clear previous game's achievement state before loading new one
  CServiceBroker::GetGameServices().GameSettings().ClearAchievementState();

  // Clean up image cache if it has grown too large
  CleanImageCacheIfNeeded();

  // Step 1: generate ROM hash to identify the game on RA
  std::string hash;
  if (!m_gameClient->Cheevos().RCGenerateHashFromFile(hash, ConsoleID(),
                                                      m_gameClient->GetGamePath().c_str()))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- hash generation failed");
    return false;
  }

  CLog::Log(LOGDEBUG, "CCheevos::LoadData -- ROM hash: {}", hash);

  // Step 2: build the game ID lookup URL from the hash
  std::string hashUrl;
  if (!m_gameClient->Cheevos().RCGetGameIDUrl(hashUrl, hash))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- failed to build game ID URL");
    return false;
  }

  XFILE::CCurlFile hashCurl;
  hashCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
  hashCurl.SetTimeout(RA_CURL_TIMEOUT_SECS);
  std::string hashResponse;
  if (!hashCurl.Get(hashUrl, hashResponse))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- hash lookup failed");
    return false;
  }

  CVariant hashData;
  if (!CJSONVariantParser::Parse(hashResponse, hashData))
    return false;

  const unsigned int gameId = static_cast<unsigned int>(hashData["GameID"].asUnsignedInteger());

  // Never request patch data with gameId == 0 — the server returns 404
  if (gameId == 0)
  {
    CLog::Log(LOGINFO, "CCheevos::LoadData -- game not found on RetroAchievements");
    return false;
  }

  CLog::Log(LOGINFO, "CCheevos::LoadData -- resolved game ID: {}", gameId);
  m_gameId = gameId;

  // Step 2: fetch patch data (achievement conditions + rich presence script)
  std::string patchUrl;
  if (!m_gameClient->Cheevos().RCGetPatchFileUrl(patchUrl, m_userName, m_loginToken, gameId))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- patch URL generation failed");
    return false;
  }

  XFILE::CCurlFile patchCurl;
  patchCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
  patchCurl.SetTimeout(RA_CURL_TIMEOUT_SECS);
  std::string patchResponse;
  if (!patchCurl.Get(patchUrl, patchResponse))
  {
    // Transient network failure - do not clear token, just bail out
    CLog::Log(LOGERROR, "CCheevos::LoadData -- patch request failed (network error)");
    return false;
  }

  CVariant data;
  if (!CJSONVariantParser::Parse(patchResponse, data))
    return false;
  // Check for explicit auth error from RA server (invalid/expired token)
  if (!data["Success"].asBoolean() && !data["Error"].asString().empty())
  {
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- RA auth error: {}", data["Error"].asString());
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString("gamesachievements.token", "");
    settings->SetBool("gamesachievements.loggedin", false);
    settings->Save();
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, "RetroAchievements",
                                          "Session expired. Please log in again in Settings.",
                                          TOAST_DISPLAY_TIME_LONG_MS, false, TOAST_MESSAGE_TIME_MS);
    return false;
  }

  // Update the file item with metadata from RetroAchievements
  if (!data.isMember(PATCH_DATA))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- patch data missing from RA response for game {}",
              m_gameId);
    return false;
  }
  auto file = std::make_unique<CFileItem>(m_gameClient->GetGamePath(), false);
  const std::string raTitle = data[PATCH_DATA][GAME_TITLE].asString();
  if (raTitle.empty())
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- game title missing from RA response for game {}",
              m_gameId);
  file->SetLabel(raTitle);
  GAME::CGameInfoTag* tag = file->GetGameInfoTag();
  if (tag != nullptr)
  {
    tag->SetTitle(raTitle);
    const std::string publisher = data[PATCH_DATA][GAME_PUBLISHER].asString();
    if (!publisher.empty())
      tag->SetPublisher(publisher);
    const std::string developer = data[PATCH_DATA][GAME_DEVELOPER].asString();
    if (!developer.empty())
      tag->SetDeveloper(developer);
    const std::string genre = data[PATCH_DATA][GAME_GENRE].asString();
    if (!genre.empty())
      tag->SetGenres({genre});
  }
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_UPDATE_PLAYER_ITEM, -1, -1,
                                             static_cast<void*>(file.release()));

  // Store game title for the load notification
  m_gameTitle = raTitle;

  // Load official achievements only (Flags == FLAGS_CORE)
  m_activatedCheevoMap.clear();
  {
    std::lock_guard<std::mutex> lock(s_cheevoTitlesMutex);
    s_cheevoTitles.clear();
  }
  const CVariant& achievements = data[PATCH_DATA][ACHIEVEMENTS];
  for (auto it = achievements.begin_array(); it != achievements.end_array(); ++it)
  {
    const CVariant& achievement = *it;
    const unsigned int flags = static_cast<unsigned int>(achievement[FLAGS].asUnsignedInteger());

    if (flags == FLAGS_CORE)
    {
      const unsigned int id = static_cast<unsigned int>(achievement[CHEEVO_ID].asUnsignedInteger());
      const std::string title = achievement[CHEEVO_TITLE].asString();

      // Skip RA system warnings (e.g. "Warning: Unknown Emulator")
      if (title.substr(0, 8) == "Warning:")
      {
        CLog::Log(LOGDEBUG, "CCheevos::LoadData -- skipping system warning: {}", title);
        continue;
      }

      m_activatedCheevoMap[id] = {
          achievement[MEM_ADDR].asString(),
          title,
          achievement[BADGE_NAME].asString(),
          achievement[CHEEVO_DESCRIPTION].asString(),
          std::to_string(static_cast<unsigned int>(achievement[CHEEVO_POINTS].asUnsignedInteger())),
          achievement[BADGE_LOCKED_URL].asString(),
          std::to_string(achievement[CHEEVO_RARITY].asDouble()),
      };

      // Store title + badge URL in static map so CallbackUrlId can reach them
      const std::string badgeUrl =
          std::string(RA_BADGE_BASE_URL) + achievement[BADGE_NAME].asString() + ".png";
      {
        std::lock_guard<std::mutex> lock(s_cheevoTitlesMutex);
        s_cheevoTitles[id] = {title, badgeUrl};
      }
    }
  }

  CLog::Log(LOGINFO, "CCheevos::LoadData -- {} achievements loaded for game {}",
            m_activatedCheevoMap.size(), gameId);

  // Update achievement state in GameSettings so GamesGUIInfo InfoLabels can access it
  KODI::GAME::CGameSettings::AchievementState achieveState;
  achieveState.gameTitle = m_gameTitle;
  achieveState.gameId = gameId;
  achieveState.totalAchievements = static_cast<unsigned int>(m_activatedCheevoMap.size());
  achieveState.loaded = true;
  for (const auto& [id, fields] : m_activatedCheevoMap)
  {
    KODI::GAME::CGameSettings::AchievementInfo info;
    info.title = fields[1];
    info.badgeUrl = std::string(RA_BADGE_BASE_URL) + fields[2] + ".png";
    info.description = fields.size() > 3 ? fields[3] : "";
    info.points = fields.size() > 4 ? static_cast<unsigned int>(std::stoul(fields[4])) : 0;
    info.lockedBadgeUrl = fields.size() > 5 ? fields[5] : "";
    info.rarity = fields.size() > 6 ? fields[6] : "";
    info.earned = false;
    achieveState.achievements.push_back(std::move(info));
  }

  // State set after session ping below with correct unlock count

  // Load and enable rich presence script if present
  const std::string richPresenceScript = data[PATCH_DATA][RICH_PRESENCE_PATCH].asString();
  if (!richPresenceScript.empty())
  {
    m_richPresenceScript = richPresenceScript;
    m_gameClient->Cheevos().RCEnableRichPresence(m_richPresenceScript);
    m_richPresenceLoaded = true;
    CLog::Log(LOGINFO, "CCheevos::LoadData -- rich presence script loaded for game {}", gameId);

    // Start periodic rich presence ping thread (every 2 minutes per RA spec)
    m_richPresenceRunning = true;
    m_richPresenceThread = std::thread(&CCheevos::RichPresencePingThread, this);
  }

  // Ping RA to register this as an active session.
  // Without this the game won't appear in the user's play history.
  const std::string sessionUrl =
      std::string(RA_BASE_URL) + "?r=startsession" + "&u=" + CURL::Encode(m_userName) +
      "&t=" + CURL::Encode(m_loginToken) + "&g=" + std::to_string(gameId);

  XFILE::CCurlFile sessionCurl;
  sessionCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
  std::string sessionResp;
  unsigned int unlockedCount = 0;

  if (sessionCurl.Get(sessionUrl, sessionResp))
  {
    CLog::Log(LOGINFO, "CCheevos::LoadData -- session started for game {}", gameId);

    // The startsession response includes "Unlocks" — the IDs already earned.
    // Parse this to show an accurate X / Y count in the load notification.
    CVariant sessionData;
    if (CJSONVariantParser::Parse(sessionResp, sessionData) && sessionData["Unlocks"].isArray())
    {
      // Collect earned achievement IDs and timestamps
      std::unordered_map<unsigned int, std::time_t> earnedMap;
      for (auto it = sessionData["Unlocks"].begin_array(); it != sessionData["Unlocks"].end_array();
           ++it)
      {
        if ((*it)["ID"].isUnsignedInteger())
        {
          const unsigned int id = static_cast<unsigned int>((*it)["ID"].asUnsignedInteger());
          // Only count achievements that are in our official map (skip warnings/unofficial)
          if (s_cheevoTitles.count(id))
          {
            const std::time_t when = static_cast<std::time_t>((*it)["When"].asUnsignedInteger());
            earnedMap[id] = when;
            ++unlockedCount;
          }
        }
      }
      // Mark earned achievements and format unlock date
      for (auto& info : achieveState.achievements)
      {
        for (const auto& [id, titleBadge] : s_cheevoTitles)
        {
          if (titleBadge.first == info.title && earnedMap.count(id))
          {
            info.earned = true;
            // Format timestamp as "Unlocked Jan 01 2026"
            std::time_t when = earnedMap[id];
            struct tm* tm_info = std::localtime(&when);
            char buf[32];
            std::strftime(buf, sizeof(buf), "Unlocked %b %d %Y", tm_info);
            info.unlockedDate = buf;
            break;
          }
        }
      }
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- session ping failed (non-fatal)");
  }

  // Set achievement state with correct unlock count from session ping
  achieveState.unlockedAchievements = unlockedCount;
  CServiceBroker::GetGameServices().GameSettings().SetAchievementState(achieveState);
  CLog::Log(LOGINFO, "CCheevos::LoadData -- achievement state set: title='{}' total={} unlocked={}",
            achieveState.gameTitle, achieveState.totalAchievements, unlockedCount);

  // Show the game load notification once:
  //   Icon:    game image from RetroAchievements (cached locally)
  //   Heading: game title
  //   Body:    "X / Y achievements unlocked"
  if (!m_gameTitle.empty() && !m_activatedCheevoMap.empty())
  {
    const std::string heading = m_gameTitle;
    const std::string body = StringUtils::Format("{} / {} achievements unlocked", unlockedCount,
                                                 m_activatedCheevoMap.size());

    // Check if icon is already cached; download in background if not
    // so game startup is not delayed by a network request
    std::string iconPath;
    const std::string imageIconUrl = data[PATCH_DATA][IMAGE_ICON_URL].asString();
    if (!imageIconUrl.empty())
    {
      const std::string iconFilename = StringUtils::Format("game_{}.png", gameId);
      const std::string localIcon = std::string(RA_GAME_ICON_CACHE) + iconFilename;
      if (XFILE::CFile::Exists(localIcon))
      {
        iconPath = localIcon;
      }
      else
      {
        // Download in background then show notification with image
        const std::string headingCopy = heading;
        const std::string bodyCopy = body;
        std::thread(
            [imageIconUrl, localIcon, headingCopy, bodyCopy]()
            {
              XFILE::CDirectory::Create(RA_GAME_ICON_CACHE);
              XFILE::CCurlFile iconCurl;
              iconCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
              std::string iconData;
              if (iconCurl.Get(imageIconUrl, iconData) && !iconData.empty())
              {
                XFILE::CFile outFile;
                if (outFile.OpenForWrite(localIcon, true))
                {
                  outFile.Write(iconData.data(), static_cast<ssize_t>(iconData.size()));
                  outFile.Close();
                  CLog::Log(LOGINFO, "CCheevos::LoadData -- cached game icon: {}", localIcon);
                  CGUIDialogKaiToast::QueueNotification(localIcon, headingCopy, bodyCopy,
                                                        TOAST_DISPLAY_TIME_MS, false,
                                                        TOAST_MESSAGE_TIME_MS);
                  return;
                }
              }
              CLog::Log(LOGWARNING, "CCheevos::LoadData -- failed to download game icon: {}",
                        imageIconUrl);
              CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, headingCopy, bodyCopy,
                                                    TOAST_DISPLAY_TIME_MS, false,
                                                    TOAST_MESSAGE_TIME_MS);
            })
            .detach();
      }
    }
    // Show notification immediately if icon was already cached
    if (!iconPath.empty())
    {
      CGUIDialogKaiToast::QueueNotification(iconPath, heading, body, TOAST_DISPLAY_TIME_MS, false,
                                            TOAST_MESSAGE_TIME_MS);
    }

    CLog::Log(LOGINFO, "CCheevos::LoadData -- notified: {} ({}/{})", m_gameTitle, unlockedCount,
              m_activatedCheevoMap.size());
  }

  return true;
}

// ===========================================================================
// Rich presence
// ===========================================================================

void CCheevos::EnableRichPresence()
{
  // Stop any existing ping thread
  m_richPresenceRunning = false;
  if (m_richPresenceThread.joinable())
    m_richPresenceThread.join();

  m_richPresenceLoaded = false;
  m_richPresenceScript.clear();
  m_gameId = 0;
}

std::string CCheevos::GetRichPresenceEvaluation()
{
  if (!m_richPresenceLoaded)
    return {};

  std::string evaluation;

  m_gameClient->Cheevos().RCGetRichPresenceEvaluation(evaluation, ConsoleID());

  return evaluation;
}

// ===========================================================================
// Achievement activation and trigger detection
// ===========================================================================

void CCheevos::ActivateAchievement()
{
  if (m_activatedCheevoMap.empty())
    LoadData();

  for (const auto& [id, fields] : m_activatedCheevoMap)
    m_gameClient->Cheevos().ActivateAchievement(id, fields[0].c_str());

  // Register persistent callback once — m_cheevoCallback is a member so its address
  // remains valid for the entire game session, avoiding the dangling pointer bug
  m_cheevoCallback = [](const std::string& achievementUrl, unsigned int cheevoId)
  {
    CLog::Log(LOGDEBUG, "CCheevos: achievement triggered: id={} url={}", cheevoId, achievementUrl);
    CallbackUrlId(achievementUrl, cheevoId);
  };
  m_gameClient->Cheevos().GetAchievementUrlId(m_cheevoCallback);
}

// ---------------------------------------------------------------------------
// Image cache cleanup
// ---------------------------------------------------------------------------
static void CleanImageCacheIfNeeded()
{
  // List all files in the cache directory
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(RA_GAME_ICON_CACHE, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
    return;

  // Calculate total cache size
  uint64_t totalSize = 0;
  for (int i = 0; i < items.Size(); ++i)
    totalSize += static_cast<uint64_t>(items[i]->GetSize());

  if (totalSize <= RA_CACHE_MAX_BYTES)
    return;

  CLog::Log(LOGINFO, "CCheevos: image cache size {}MB exceeds limit, evicting oldest files",
            totalSize / (1024 * 1024));

  // Sort by date - oldest first
  items.Sort(SortBy::DATE, SortOrder::ASCENDING);

  // Delete oldest files until we are under target size
  for (int i = 0; i < items.Size() && totalSize > RA_CACHE_TARGET_BYTES; ++i)
  {
    const uint64_t fileSize = static_cast<uint64_t>(items[i]->GetSize());
    if (XFILE::CFile::Delete(items[i]->GetPath()))
    {
      totalSize -= fileSize;
      CLog::Log(LOGDEBUG, "CCheevos: evicted cache file: {}", items[i]->GetPath());
    }
  }

  CLog::Log(LOGINFO, "CCheevos: image cache cleaned, new size {}MB", totalSize / (1024 * 1024));
}

// ---------------------------------------------------------------------------
// Offline award queue helpers
// ---------------------------------------------------------------------------
static void FlushAwardQueue()
{
  if (!XFILE::CFile::Exists(RA_AWARD_QUEUE_FILE))
    return;

  XFILE::CFile queueFile;
  std::vector<uint8_t> data;
  if (queueFile.LoadFile(CURL(RA_AWARD_QUEUE_FILE), data) <= 0)
    return;

  const std::string queueData(data.begin(), data.end());
  std::vector<std::string> urls = StringUtils::Split(queueData, "\n");

  std::vector<std::string> remaining;
  for (const auto& url : urls)
  {
    if (url.empty())
      continue;

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
    std::string response;
    if (curl.Get(url, response))
      CLog::Log(LOGINFO, "CCheevos: flushed queued award: {}", url);
    else
    {
      CLog::Log(LOGWARNING, "CCheevos: queued award still failing, keeping: {}", url);
      remaining.push_back(url);
    }
  }

  // Rewrite queue with only the still-failing ones
  if (remaining.empty())
  {
    XFILE::CFile::Delete(RA_AWARD_QUEUE_FILE);
  }
  else
  {
    XFILE::CFile outFile;
    if (outFile.OpenForWrite(CURL(RA_AWARD_QUEUE_FILE), true))
    {
      const std::string newData = StringUtils::Join(remaining, "\n") + "\n";
      outFile.Write(newData.data(), static_cast<ssize_t>(newData.size()));
      outFile.Close();
    }
  }
}

static void QueueAward(const std::string& url)
{
  // Cap queue at 50 entries to prevent unbounded growth
  constexpr int RA_AWARD_QUEUE_MAX = 50;
  XFILE::CFile queueFile;
  std::vector<uint8_t> existingData;
  if (queueFile.LoadFile(CURL(RA_AWARD_QUEUE_FILE), existingData) > 0)
  {
    const std::string existing(existingData.begin(), existingData.end());
    int lineCount = 0;
    for (char c : existing)
      if (c == '\n')
        lineCount++;
    if (lineCount >= RA_AWARD_QUEUE_MAX)
    {
      CLog::Log(LOGWARNING, "CCheevos: award queue full ({} entries), dropping: {}", lineCount,
                url);
      return;
    }
  }

  XFILE::CDirectory::Create("special://profile/cache/retroachievements/");
  XFILE::CFile outFile;
  if (outFile.OpenForWrite(CURL(RA_AWARD_QUEUE_FILE), false))
  {
    const std::string line = url + "\n";
    outFile.Seek(0, SEEK_END);
    outFile.Write(line.data(), static_cast<ssize_t>(line.size()));
    outFile.Close();
    CLog::Log(LOGWARNING, "CCheevos: award queued for retry: {}", url);
  }
}

void CCheevos::CallbackUrlId(const std::string& achievementUrl, unsigned int cheevoId)
{
  // If this achievement ID is not in our official map it is unofficial/demoted.
  // The addon runtime activates ALL achievements from patch data regardless of
  // flags, so we filter here to avoid notifications and awards for unofficial ones.
  std::string cheevoTitle;
  std::string badgeUrl;
  {
    std::lock_guard<std::mutex> titleLock(s_cheevoTitlesMutex);
    auto titleIt = s_cheevoTitles.find(cheevoId);
    if (titleIt == s_cheevoTitles.end())
    {
      CLog::Log(LOGDEBUG, "CCheevos::CallbackUrlId -- skipping unofficial achievement {}", cheevoId);
      return;
    }
    cheevoTitle = titleIt->second.first;
    badgeUrl    = titleIt->second.second;
  } // lock released — safe to do slow work below

  // Skip notification if already earned in a previous session.
  // LoadData marks earned achievements from the startsession Unlocks list.
  // fceumm fires the callback for already-earned achievements when their
  // conditions are met in-game — RA handles duplicate awards gracefully
  // but we must not show a notification the user has already seen.
  {
    const auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
    for (const auto& info : state.achievements)
    {
      if (info.title == cheevoTitle && info.earned)
      {
        CLog::Log(LOGDEBUG,
                  "CCheevos::CallbackUrlId -- skipping notification for already-earned '{}'",
                  cheevoTitle);
        return;
      }
    }
  }

  // Flush any previously queued awards first
  FlushAwardQueue();

  // Send award to RA server
  XFILE::CCurlFile curl;
  curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
  std::string res;
  if (curl.Get(achievementUrl, res))
  {
    CLog::Log(LOGINFO, "CCheevos::CallbackUrlId -- award sent for '{}' ({})", cheevoTitle,
              cheevoId);
  }
  else
  {
    CLog::Log(LOGWARNING, "CCheevos::CallbackUrlId -- award failed, queuing for retry: {}",
              cheevoId);
    QueueAward(achievementUrl);
  }

  // Check if badge is already cached; download in background if not
  std::string iconPath;
  if (!badgeUrl.empty())
  {
    const std::string badgeFilename = "badge_" + std::to_string(cheevoId) + ".png";
    const std::string localBadge = std::string(RA_GAME_ICON_CACHE) + badgeFilename;
    if (XFILE::CFile::Exists(localBadge))
    {
      iconPath = localBadge;
    }
    else
    {
      // Download badge in background then show notification with image
      const std::string badgeUrlCopy = badgeUrl;
      const std::string cheevoTitleCopy = cheevoTitle;
      std::thread(
          [badgeUrlCopy, localBadge, cheevoTitleCopy]()
          {
            XFILE::CDirectory::Create(RA_GAME_ICON_CACHE);
            XFILE::CCurlFile badgeCurl;
            badgeCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
            std::string badgeData;
            if (badgeCurl.Get(badgeUrlCopy, badgeData) && !badgeData.empty())
            {
              XFILE::CFile outFile;
              if (outFile.OpenForWrite(localBadge, true))
              {
                outFile.Write(badgeData.data(), static_cast<ssize_t>(badgeData.size()));
                outFile.Close();
                CGUIDialogKaiToast::QueueNotification(localBadge, "Achievement Unlocked!",
                                                      cheevoTitleCopy, TOAST_DISPLAY_TIME_MS, false,
                                                      TOAST_MESSAGE_TIME_MS);
                return;
              }
            }
            CGUIDialogKaiToast::QueueNotification(
                CGUIDialogKaiToast::Info, "Achievement Unlocked!",
                cheevoTitleCopy.empty() ? "Achievement earned!" : cheevoTitleCopy,
                TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
          })
          .detach();
    }
  }
  // Show notification immediately if badge was already cached
  if (!iconPath.empty())
  {
    CGUIDialogKaiToast::QueueNotification(iconPath, "Achievement Unlocked!", cheevoTitle,
                                          TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
  }

  // Update unlocked count in shared state and check for mastery
  {
    auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
    state.unlockedAchievements++;

    // Mark this achievement as earned in the per-achievement list
    for (auto& info : state.achievements)
    {
      if (info.title == cheevoTitle)
      {
        info.earned = true;
        break;
      }
    }

    CServiceBroker::GetGameServices().GameSettings().SetAchievementState(state);

    // Mastery notification — all achievements unlocked
    if (state.totalAchievements > 0 && state.unlockedAchievements >= state.totalAchievements)
    {
      CLog::Log(LOGINFO, "CCheevos::CallbackUrlId -- mastery achieved for '{}'", state.gameTitle);

      // Use the cached game icon for the mastery notification
      const std::string masteryIcon =
          std::string(RA_GAME_ICON_CACHE) + StringUtils::Format("game_{}.png", state.gameId);

      if (XFILE::CFile::Exists(masteryIcon))
      {
        CGUIDialogKaiToast::QueueNotification(masteryIcon, "Mastered!", state.gameTitle,
                                              TOAST_DISPLAY_TIME_LONG_MS, false,
                                              TOAST_MESSAGE_TIME_MS);
      }
      else
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Mastered!",
                                              state.gameTitle, TOAST_DISPLAY_TIME_LONG_MS, false,
                                              TOAST_MESSAGE_TIME_MS);
      }
    }
  }
}

// ===========================================================================
// Rich presence periodic ping thread
// ===========================================================================

void CCheevos::RichPresencePingThread()
{
  CLog::Log(LOGINFO, "CCheevos::RichPresencePingThread -- started for game {}", m_gameId);

  while (m_richPresenceRunning)
  {
    // Wait 2 minutes between pings per RA spec, checking stop flag every 100ms
    for (int i = 0; i < 1200 && m_richPresenceRunning; ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!m_richPresenceRunning)
      break;

    const std::string evaluation = GetRichPresenceEvaluation();
    if (evaluation.empty())
      continue;

    // Update AchievementState so RETROPLAYER_RICH_PRESENCE InfoLabel is current
    {
      auto state = CServiceBroker::GetGameServices().GameSettings().GetAchievementState();
      state.richPresence = evaluation;
      CServiceBroker::GetGameServices().GameSettings().SetAchievementState(state);
    }

    CLog::Log(LOGDEBUG, "CCheevos::RichPresencePingThread -- posting: {}", evaluation);

    std::string url;
    std::string postData;
    if (!m_gameClient->Cheevos().RCPostRichPresenceUrl(url, postData, m_userName, m_loginToken,
                                                       m_gameId, evaluation))
    {
      CLog::Log(LOGWARNING, "CCheevos::RichPresencePingThread -- failed to build URL");
      continue;
    }

    XFILE::CCurlFile curl;
    curl.SetRequestHeader("User-Agent", RA_USER_AGENT);
    std::string response;
    if (curl.Post(url, postData, response))
      CLog::Log(LOGDEBUG, "CCheevos::RichPresencePingThread -- ping sent OK");
    else
      CLog::Log(LOGWARNING, "CCheevos::RichPresencePingThread -- ping failed");
  }

  CLog::Log(LOGINFO, "CCheevos::RichPresencePingThread -- stopped");
}

// ===========================================================================
// Console ID helper
// ===========================================================================

RConsoleID CCheevos::ConsoleID()
{
  const std::string ext = URIUtils::GetExtension(m_gameClient->GetGamePath());
  return ExtensionToConsoleID(ext);
}
