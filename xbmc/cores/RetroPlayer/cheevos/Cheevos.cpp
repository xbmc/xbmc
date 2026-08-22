/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Cheevos.h"

#include "CheevosAwardQueue.h"
#include "CheevosImageCache.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "XBDateTime.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/tags/GameInfoTag.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace KODI;
using namespace RETRO;
using namespace KODI::GAME;

namespace
{

// RA Connect API base URL
constexpr auto RA_BASE_URL = "https://retroachievements.org/dorequest.php";
constexpr auto RA_BADGE_BASE_URL = "https://i.retroachievements.org/Badge/";

// KaiToast display timings (milliseconds)
constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr int RA_CURL_TIMEOUT_SECS = 10;
constexpr std::size_t RA_DOWNLOAD_QUEUE_MAX = 32;
constexpr unsigned int TOAST_DISPLAY_TIME_LONG_MS = 8000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

// JSON field names
constexpr auto PATCH_DATA = "PatchData";
constexpr auto GAME_TITLE = "Title";
constexpr auto GAME_PUBLISHER = "Publisher";
constexpr auto GAME_DEVELOPER = "Developer";
constexpr auto GAME_GENRE = "Genre";
constexpr auto IMAGE_ICON_URL = "ImageIconURL";
constexpr unsigned int RA_MAX_ACHIEVEMENTS = 10000;
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

static bool ToUnsignedInt(const CVariant& value, unsigned int& result)
{
  if (!value.isInteger() || (value.isSignedInteger() && value.asInteger() < 0))
    return false;

  const uint64_t unsignedValue = value.asUnsignedInteger();
  if (unsignedValue > std::numeric_limits<unsigned int>::max())
    return false;

  result = static_cast<unsigned int>(unsignedValue);
  return true;
}

} // namespace

CCheevos::CCheevos(GAME::CGameClient* gameClient,
                   const std::string& userName,
                   const std::string& loginToken)
  : RA_USER_AGENT(CSysInfo::GetUserAgent()),
    m_gameClient(gameClient),
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

  // Start background work only after potentially-throwing initialization is complete
  m_downloadThread = std::thread(&CCheevos::DownloadThread, this);
}

CCheevos::~CCheevos()
{
  Stop();
}

void CCheevos::Stop()
{
  m_richPresenceRunning = false;
  if (m_richPresenceThread.joinable())
    m_richPresenceThread.join();

  {
    std::lock_guard<std::mutex> lock(m_downloadThreadsMutex);
    m_downloadRunning = false;
    m_downloadQueue.clear();
  }
  m_downloadCondition.notify_one();
  if (m_downloadThread.joinable())
    m_downloadThread.join();
}

bool CCheevos::RCLogin(const std::string& password)
{
  if (m_userName.empty() || password.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- username or password is empty");
    return false;
  }

  // Keep credentials out of the URL so they are not recorded in access logs
  const std::string loginUrl = std::string(RA_BASE_URL) + "?r=login2";
  const std::string postData = "u=" + CURL::Encode(m_userName) + "&p=" + CURL::Encode(password);

  XFILE::CCurlFile curl;
  curl.SetRequestHeader("User-Agent", RA_USER_AGENT);

  CLog::Log(LOGDEBUG, "CCheevos::RCLogin -- posting login request to {} as '{}'", RA_BASE_URL,
            m_userName);

  std::string response;
  if (!curl.Post(loginUrl, postData, response) || response.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::RCLogin -- HTTP POST failed (network error)");
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

  // Persist token for next session. NEVER persist the password
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetString("gamesachievements.username", m_userName);
  settings->SetString("gamesachievements.token", m_loginToken);

  // Push credentials to the game addon layer
  m_gameClient->Cheevos().SetRetroAchievementsCredentials(m_userName.c_str(), m_loginToken.c_str());

  CLog::Log(LOGINFO, "CCheevos::RCLogin -- successfully logged in as '{}'", m_userName);
  return true;
}

void CCheevos::ResetRuntime()
{
  m_gameClient->Cheevos().RCResetRuntime();
}

bool CCheevos::LoadData()
{
  if (m_userName.empty() || m_loginToken.empty())
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- not logged in");
    return false;
  }

  // Clear previous game's achievement state before loading new one
  CServiceBroker::GetGameServices().AchievementRuntime().Clear();

  // Clean up image cache if it has grown too large
  CCheevosImageCache::CleanIfNeeded();

  // Generate ROM hash to identify the game on RA
  std::string hash;
  if (!m_gameClient->Cheevos().RCGenerateHashFromFile(hash, ConsoleID(),
                                                      m_gameClient->GetGamePath().c_str()))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- hash generation failed");
    return false;
  }

  CLog::Log(LOGDEBUG, "CCheevos::LoadData -- ROM hash: {}", hash);

  // Build the game ID lookup URL from the hash
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
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- failed to parse game ID response");
    return false;
  }

  unsigned int gameId = 0;
  if (!ToUnsignedInt(hashData["GameID"], gameId))
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- invalid game ID in hash response");
    return false;
  }

  // Never request patch data with gameId == 0 — the server returns 404
  if (gameId == 0)
  {
    CLog::Log(LOGINFO, "CCheevos::LoadData -- game not found on RetroAchievements");
    return false;
  }

  CLog::Log(LOGINFO, "CCheevos::LoadData -- resolved game ID: {}", gameId);
  m_gameId = gameId;

  // Fetch patch data (achievement conditions + rich presence script)
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
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- failed to parse achievement patch response");
    return false;
  }

  // Check for explicit auth error from RA server (invalid/expired token)
  if (!data["Success"].asBoolean() && !data["Error"].asString().empty())
  {
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- RA auth error: {}", data["Error"].asString());

    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetString("gamesachievements.token", "");
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

  const CVariant& achievements = data[PATCH_DATA][ACHIEVEMENTS];
  if (!achievements.isArray())
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- achievement list missing from RA response");
    return false;
  }

  if (achievements.size() > RA_MAX_ACHIEVEMENTS)
  {
    CLog::Log(LOGERROR, "CCheevos::LoadData -- achievement count {} exceeds supported limit {}",
              achievements.size(), RA_MAX_ACHIEVEMENTS);
    return false;
  }

  auto file = std::make_unique<CFileItem>(m_gameClient->GetGamePath(), false);

  const std::string raTitle = data[PATCH_DATA][GAME_TITLE].asString();
  if (raTitle.empty())
  {
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- game title missing from RA response for game {}",
              m_gameId);
  }
  file->SetLabel(raTitle);

  if (GAME::CGameInfoTag* tag = file->GetGameInfoTag(); tag != nullptr)
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

  // The handler adopts the pointer on success. Reclaim it if shutdown prevents dispatch.
  CFileItem* filePtr = file.release();
  if (CServiceBroker::GetAppMessenger()->SendMsg(TMSG_UPDATE_PLAYER_ITEM, -1, -1,
                                                 static_cast<void*>(filePtr)) < 0)
  {
    file.reset(filePtr);
    CLog::Log(LOGWARNING, "CCheevos::LoadData -- failed to publish game metadata");
  }

  // Store game title for the load notification
  m_gameTitle = raTitle;

  // Load official/core achievements only (Flags == 3)
  ActivatedCheevoMap activatedCheevoMap;
  CheevoTitleMap cheevoTitles;

  for (auto it = achievements.begin_array(); it != achievements.end_array(); ++it)
  {
    const CVariant& achievement = *it;
    const uint64_t flags = achievement[FLAGS].asUnsignedInteger();

    if (flags == FLAGS_CORE)
    {
      unsigned int id = 0;
      if (!ToUnsignedInt(achievement[CHEEVO_ID], id))
      {
        CLog::Log(LOGWARNING, "CCheevos::LoadData -- skipping achievement with invalid ID");
        continue;
      }

      const std::string title = achievement[CHEEVO_TITLE].asString();

      // Skip RA system warnings (e.g. "Warning: Unknown Emulator")
      if (title.compare(0, 8, "Warning:") == 0)
      {
        CLog::Log(LOGDEBUG, "CCheevos::LoadData -- skipping system warning: {}", title);
        continue;
      }

      unsigned int points = 0;
      if (!ToUnsignedInt(achievement[CHEEVO_POINTS], points))
      {
        CLog::Log(LOGWARNING,
                  "CCheevos::LoadData -- invalid points value for achievement {}, using 0", id);
      }

      activatedCheevoMap[id] = {
          achievement[MEM_ADDR].asString(),
          title,
          achievement[BADGE_NAME].asString(),
          achievement[CHEEVO_DESCRIPTION].asString(),
          std::to_string(points),
          achievement[BADGE_LOCKED_URL].asString(),
          std::to_string(achievement[CHEEVO_RARITY].asDouble()),
      };

      // Store title + badge URL so the achievement callback can look them up
      const std::string badgeUrl =
          std::string(RA_BADGE_BASE_URL) + achievement[BADGE_NAME].asString() + ".png";
      cheevoTitles[id] = {title, badgeUrl};
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_activatedCheevoMutex);
    m_activatedCheevoMap = activatedCheevoMap;
  }

  {
    std::lock_guard<std::mutex> lock(m_cheevoTitlesMutex);
    m_cheevoTitles = cheevoTitles;
  }

  CLog::Log(LOGINFO, "CCheevos::LoadData -- {} achievements loaded for game {}",
            activatedCheevoMap.size(), gameId);

  // Publish achievement state so GamesGUIInfo InfoLabels can access it
  AchievementState achieveState;
  achieveState.gameTitle = m_gameTitle;
  achieveState.gameId = gameId;
  achieveState.totalAchievements = static_cast<unsigned int>(activatedCheevoMap.size());
  achieveState.loaded = true;
  for (const auto& [id, fields] : activatedCheevoMap)
  {
    AchievementInfo info;
    info.id = id;
    info.title = fields[1];
    info.badgeUrl = std::string(RA_BADGE_BASE_URL) + fields[2] + ".png";
    info.description = fields.size() > 3 ? fields[3] : "";
    if (fields.size() > 4)
    {
      try
      {
        const unsigned long points = std::stoul(fields[4]);
        if (points > std::numeric_limits<unsigned int>::max())
          throw std::out_of_range("achievement points");
        info.points = static_cast<unsigned int>(points);
      }
      catch (const std::exception&)
      {
        CLog::Log(LOGWARNING,
                  "CCheevos::LoadData -- invalid points value '{}' for achievement {}, using 0",
                  fields[4], id);
        info.points = 0;
      }
    }
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

  // Ping RA to register this as an active session. Without this the game won't
  // appear in the user's play history.
  const std::string sessionUrl =
      std::string(RA_BASE_URL) + "?r=startsession" + "&u=" + CURL::Encode(m_userName) +
      "&t=" + CURL::Encode(m_loginToken) + "&g=" + std::to_string(gameId);

  XFILE::CCurlFile sessionCurl;
  sessionCurl.SetRequestHeader("User-Agent", RA_USER_AGENT);
  std::string sessionResp;
  unsigned int unlockedCount = 0;
  bool unlockCountKnown = false;

  if (sessionCurl.Get(sessionUrl, sessionResp))
  {
    CLog::Log(LOGINFO, "CCheevos::LoadData -- session started for game {}", gameId);

    // The startsession response includes "Unlocks" — the IDs already earned.
    // Parse this to show an accurate X / Y count in the load notification.
    CVariant sessionData;
    if (CJSONVariantParser::Parse(sessionResp, sessionData) && sessionData["Unlocks"].isArray())
    {
      unlockCountKnown = true;
      // Collect earned achievement IDs and timestamps
      std::unordered_map<unsigned int, std::time_t> earnedMap;
      for (auto it = sessionData["Unlocks"].begin_array(); it != sessionData["Unlocks"].end_array();
           ++it)
      {
        unsigned int id = 0;
        if (ToUnsignedInt((*it)["ID"], id))
        {
          // Only count achievements that are in our official map (skip warnings/unofficial)
          if (cheevoTitles.count(id))
          {
            const uint64_t timestamp = (*it)["When"].asUnsignedInteger();
            if (timestamp > static_cast<uint64_t>(std::numeric_limits<std::time_t>::max()))
            {
              CLog::Log(LOGWARNING,
                        "CCheevos::LoadData -- ignoring invalid unlock time for achievement {}",
                        id);
              continue;
            }

            const std::time_t when = static_cast<std::time_t>(timestamp);
            if (earnedMap.emplace(id, when).second)
              ++unlockedCount;
          }
        }
      }

      // Mark earned achievements and format unlock dates by achievement ID
      for (auto& info : achieveState.achievements)
      {
        const auto earnedIt = earnedMap.find(info.id);
        if (earnedIt != earnedMap.end())
        {
          info.earned = true;

          // Format timestamp as "Unlocked Jan 01 2026"
          const CDateTime unlockedDate(earnedIt->second);
          info.unlockedDate = "Unlocked " + unlockedDate.GetAsLocalizedDate("MMM dd yyyy");
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
  CServiceBroker::GetGameServices().AchievementRuntime().SetState(achieveState);
  CLog::Log(LOGINFO, "CCheevos::LoadData -- achievement state set: title='{}' total={} unlocked={}",
            achieveState.gameTitle, achieveState.totalAchievements, unlockedCount);

  // Show the game load notification once:
  //   Icon:    game image from RetroAchievements (cached locally)
  //   Heading: game title
  //   Body:    "X / Y achievements unlocked"
  if (!m_gameTitle.empty() && !activatedCheevoMap.empty())
  {
    const std::string heading = m_gameTitle;
    const std::string body =
        unlockCountKnown
            ? StringUtils::Format("{} / {} achievements unlocked", unlockedCount,
                                  activatedCheevoMap.size())
            : StringUtils::Format("{} achievements available", activatedCheevoMap.size());

    // Check if icon is already cached; download in background if not
    // so game startup is not delayed by a network request
    std::string iconPath;
    const std::string imageIconUrl = data[PATCH_DATA][IMAGE_ICON_URL].asString();
    if (!imageIconUrl.empty())
    {
      const std::string localIcon = CCheevosImageCache::GetGameIconPath(gameId);
      if (CCheevosImageCache::IsCached(localIcon))
      {
        iconPath = localIcon;
      }
      else
      {
        // Download in background then show notification with image
        const std::string headingCopy = heading;
        const std::string bodyCopy = body;
        const std::string userAgent = RA_USER_AGENT;
        if (!QueueDownload(
                [imageIconUrl, localIcon, headingCopy, bodyCopy, userAgent]()
                {
                  XFILE::CCurlFile iconCurl;
                  iconCurl.SetRequestHeader("User-Agent", userAgent);
                  iconCurl.SetTimeout(RA_CURL_TIMEOUT_SECS);
                  std::string iconData;
                  if (iconCurl.Get(imageIconUrl, iconData) &&
                      CCheevosImageCache::Store(localIcon, iconData))
                  {
                    CLog::Log(LOGINFO, "CCheevos::LoadData -- cached game icon: {}", localIcon);
                    CGUIDialogKaiToast::QueueNotification(localIcon, headingCopy, bodyCopy,
                                                          TOAST_DISPLAY_TIME_MS, false,
                                                          TOAST_MESSAGE_TIME_MS);
                    return;
                  }
                  CLog::Log(LOGWARNING, "CCheevos::LoadData -- failed to download game icon: {}",
                            imageIconUrl);
                  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, headingCopy,
                                                        bodyCopy, TOAST_DISPLAY_TIME_MS, false,
                                                        TOAST_MESSAGE_TIME_MS);
                }))
        {
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, headingCopy, bodyCopy,
                                                TOAST_DISPLAY_TIME_MS, false,
                                                TOAST_MESSAGE_TIME_MS);
        }
      }
    }

    // Show notification immediately if icon was already cached
    if (!iconPath.empty())
    {
      CGUIDialogKaiToast::QueueNotification(iconPath, heading, body, TOAST_DISPLAY_TIME_MS, false,
                                            TOAST_MESSAGE_TIME_MS);
    }

    CLog::Log(LOGINFO, "CCheevos::LoadData -- notified: {} ({}/{})", m_gameTitle, unlockedCount,
              activatedCheevoMap.size());
  }

  return true;
}

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

void CCheevos::ActivateAchievement()
{
  ActivatedCheevoMap activatedCheevoMap;
  {
    std::lock_guard<std::mutex> lock(m_activatedCheevoMutex);
    activatedCheevoMap = m_activatedCheevoMap;
  }

  if (activatedCheevoMap.empty())
  {
    LoadData();

    std::lock_guard<std::mutex> lock(m_activatedCheevoMutex);
    activatedCheevoMap = m_activatedCheevoMap;
  }

  for (const auto& [id, fields] : activatedCheevoMap)
    m_gameClient->Cheevos().ActivateAchievement(id, fields[0].c_str());

  // Register persistent callback once — m_cheevoCallback is a member so its address
  // remains valid for the entire game session, avoiding the dangling pointer bug
  m_cheevoCallback = [this](const std::string& achievementUrl, unsigned int cheevoId)
  {
    CLog::Log(LOGDEBUG, "CCheevos: achievement triggered: id={} url={}", cheevoId, achievementUrl);
    CallbackUrlId(achievementUrl, cheevoId);
  };
  m_gameClient->Cheevos().GetAchievementUrlId(m_cheevoCallback);
}

void CCheevos::CallbackUrlId(const std::string& achievementUrl, unsigned int cheevoId)
{
  // If this achievement ID is not in our official map it is unofficial/demoted.
  // The addon runtime activates ALL achievements from patch data regardless of
  // flags, so we filter here to avoid notifications and awards for unofficial ones.
  std::string cheevoTitle;
  std::string badgeUrl;
  {
    std::lock_guard<std::mutex> titleLock(m_cheevoTitlesMutex);
    const auto titleIt = m_cheevoTitles.find(cheevoId);
    if (titleIt == m_cheevoTitles.end())
    {
      CLog::Log(LOGDEBUG, "CCheevos::CallbackUrlId -- skipping unofficial achievement {}",
                cheevoId);
      return;
    }

    cheevoTitle = titleIt->second.first;
    badgeUrl = titleIt->second.second;
  }

  // Skip notification if already earned in a previous session.
  // LoadData marks earned achievements from the startsession Unlocks list.
  // fceumm fires the callback for already-earned achievements when their
  // conditions are met in-game — RA handles duplicate awards gracefully
  // but we must not show a notification the user has already seen.
  {
    const auto state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();
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
  CCheevosAwardQueue::Flush();

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
    CCheevosAwardQueue::Queue(achievementUrl);
  }

  // Check if badge is already cached; download in background if not
  std::string iconPath;
  if (!badgeUrl.empty())
  {
    const std::string localBadge = CCheevosImageCache::GetBadgePath(cheevoId);
    if (CCheevosImageCache::IsCached(localBadge))
    {
      iconPath = localBadge;
    }
    else
    {
      // Download badge in background then show notification with image
      const std::string badgeUrlCopy = badgeUrl;
      const std::string cheevoTitleCopy = cheevoTitle;
      const std::string userAgent = RA_USER_AGENT;
      if (!QueueDownload(
              [badgeUrlCopy, localBadge, cheevoTitleCopy, userAgent]()
              {
                XFILE::CCurlFile badgeCurl;
                badgeCurl.SetRequestHeader("User-Agent", userAgent);
                badgeCurl.SetTimeout(RA_CURL_TIMEOUT_SECS);

                std::string badgeData;
                if (badgeCurl.Get(badgeUrlCopy, badgeData) &&
                    CCheevosImageCache::Store(localBadge, badgeData))
                {
                  CGUIDialogKaiToast::QueueNotification(localBadge, "Achievement Unlocked!",
                                                        cheevoTitleCopy, TOAST_DISPLAY_TIME_MS,
                                                        false, TOAST_MESSAGE_TIME_MS);
                  return;
                }

                CGUIDialogKaiToast::QueueNotification(
                    CGUIDialogKaiToast::Info, "Achievement Unlocked!",
                    cheevoTitleCopy.empty() ? "Achievement earned!" : cheevoTitleCopy,
                    TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
              }))
      {
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Achievement Unlocked!",
                                              cheevoTitleCopy.empty() ? "Achievement earned!"
                                                                      : cheevoTitleCopy,
                                              TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
      }
    }
  }

  // Show notification immediately if badge was already cached
  if (!iconPath.empty())
  {
    CGUIDialogKaiToast::QueueNotification(iconPath, "Achievement Unlocked!", cheevoTitle,
                                          TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
  }

  // Atomically mark the achievement earned and update the shared unlocked count
  {
    bool newlyEarned = false;
    const auto state =
        CServiceBroker::GetGameServices().AchievementRuntime().MarkEarned(cheevoId, newlyEarned);

    // Mastery notification — all achievements unlocked
    if (newlyEarned && state.totalAchievements > 0 &&
        state.unlockedAchievements >= state.totalAchievements)
    {
      CLog::Log(LOGINFO, "CCheevos::CallbackUrlId -- mastery achieved for '{}'", state.gameTitle);

      // Use the cached game icon for the mastery notification
      const std::string masteryIcon = CCheevosImageCache::GetGameIconPath(state.gameId);

      if (CCheevosImageCache::IsCached(masteryIcon))
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
    CServiceBroker::GetGameServices().AchievementRuntime().SetRichPresence(evaluation);
    if (evaluation.empty())
      continue;

    // Set caption of playing item
    std::unique_ptr<CFileItem> file{std::make_unique<CFileItem>()};

    GAME::CGameInfoTag& tag = *file->GetGameInfoTag();
    tag.SetCaption(evaluation);

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_UPDATE_PLAYER_ITEM, -1, -1,
                                               static_cast<void*>(file.release()));

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

void CCheevos::DownloadThread()
{
  while (true)
  {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(m_downloadThreadsMutex);
      m_downloadCondition.wait(lock,
                               [this] { return !m_downloadRunning || !m_downloadQueue.empty(); });
      if (!m_downloadRunning)
        return;

      task = std::move(m_downloadQueue.front());
      m_downloadQueue.pop_front();
    }

    try
    {
      task();
    }
    catch (const std::exception& exception)
    {
      CLog::Log(LOGERROR, "CCheevos::DownloadThread -- download task failed: {}", exception.what());
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "CCheevos::DownloadThread -- download task failed");
    }
  }
}

bool CCheevos::QueueDownload(std::function<void()> task)
{
  {
    std::lock_guard<std::mutex> lock(m_downloadThreadsMutex);
    if (!m_downloadRunning || m_downloadQueue.size() >= RA_DOWNLOAD_QUEUE_MAX)
      return false;
    m_downloadQueue.emplace_back(std::move(task));
  }
  m_downloadCondition.notify_one();
  return true;
}

RConsoleID CCheevos::ConsoleID()
{
  const std::string ext = URIUtils::GetExtension(m_gameClient->GetGamePath());
  return ExtensionToConsoleID(ext);
}
