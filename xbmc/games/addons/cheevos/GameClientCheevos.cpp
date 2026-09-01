/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientCheevos.h"

#include "ServiceBroker.h"
#include "games/dialogs/osd/DialogGameIndicators.h"
#include "TextureCache.h"
#include "XBDateTime.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <utility>
#include <vector>

namespace
{
// The add-on is not trusted to send sane counts. Published sets are an order
// of magnitude below these.
constexpr size_t MAX_ACHIEVEMENTS = 4096;

constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

std::string SafeString(const char* str)
{
  return str != nullptr ? std::string{str} : std::string{};
}

std::string Localize(uint32_t stringId)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(stringId);
}

/*!
 * \brief Ask the achievements dialog to rebuild itself
 *
 * Called from the add-on's thread, so the message is queued rather than sent.
 * The dialog ignores it when it isn't open.
 */
void NotifyDialogs()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_DIALOG_GAME_ACHIEVEMENTS, 0, GUI_MSG_REFRESH_LIST);
  gui->GetWindowManager().SendThreadMessage(msg, WINDOW_DIALOG_GAME_ACHIEVEMENTS);
}

/*!
 * \brief Convert a Unix timestamp to a local date, invalid if unset
 *
 * Converted rather than constructed directly: the dialog formats the date
 * without further conversion, so UTC fields would show the wrong day either
 * side of midnight.
 */
CDateTime UnlockTime(int64_t unixTime)
{
  if (unixTime <= 0)
    return {};

  return CDateTime::FromUTCDateTime(static_cast<time_t>(unixTime));
}
} // namespace

using namespace KODI;
using namespace GAME;

CGameClientCheevos::CGameClientCheevos(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient),
    m_struct(addonStruct)
{
}

void CGameClientCheevos::OnGameLoaded(const game_rc_game_loaded& data)
{
  const std::string gameTitle = SafeString(data.title);

  const size_t achievementCount = data.achievements != nullptr ? data.achievement_count : 0;

  if (achievementCount > MAX_ACHIEVEMENTS)
  {
    CLog::Log(LOGWARNING, "CGameClientCheevos: game {} reported {} achievements, truncating to {}",
              data.game_id, achievementCount, MAX_ACHIEVEMENTS);
  }

  AchievementState achievementState;
  achievementState.gameTitle = gameTitle;
  achievementState.gameId = data.game_id;
  achievementState.loaded = true;
  achievementState.achievements.reserve(std::min(achievementCount, MAX_ACHIEVEMENTS));

  for (size_t i = 0; i < std::min(achievementCount, MAX_ACHIEVEMENTS); ++i)
  {
    const game_rc_achievement& achievement = data.achievements[i];

    AchievementInfo info;
    info.id = achievement.id;
    info.title = SafeString(achievement.title);
    info.description = SafeString(achievement.description);
    info.badgeUrl = SafeString(achievement.badge_url);
    info.lockedBadgeUrl = SafeString(achievement.badge_locked_url);
    info.rarity = std::clamp(achievement.rarity, 0.0f, 100.0f);
    info.points = achievement.points;
    info.earned = achievement.unlock_state != GAME_RC_UNLOCK_STATE_LOCKED;
    if (info.earned)
      info.unlockedDate = UnlockTime(achievement.unlock_time);

    if (info.earned)
      ++achievementState.unlockedAchievements;

    achievementState.achievements.emplace_back(std::move(info));
  }

  achievementState.totalAchievements =
      static_cast<unsigned int>(achievementState.achievements.size());

  CLog::Log(LOGINFO, "CGameClientCheevos: loaded game {} \"{}\" with {} achievements ({} earned)",
            data.game_id, gameTitle, achievementState.totalAchievements,
            achievementState.unlockedAchievements);

  CServiceBroker::GetGameServices().AchievementRuntime().SetState(achievementState);

  // Warm the cache with the unlocked badges of still-locked achievements: an
  // unlock toast is queued before its image is fetched, and the notification
  // window redraws its last texture until the download lands.
  for (const AchievementInfo& info : achievementState.achievements)
  {
    if (!info.earned && !info.badgeUrl.empty())
      CServiceBroker::GetTextureCache()->BackgroundCacheImage(info.badgeUrl);
  }

  NotifyDialogs();

  if (achievementState.totalAchievements == 0)
    return;

  // "{0:d} of {1:d} achievements unlocked"
  const std::string description = StringUtils::Format(
      Localize(35284), achievementState.unlockedAchievements, achievementState.totalAchievements);

  // Kodi's texture cache resolves remote URLs; an empty path falls back to
  // the default icon
  CGUIDialogKaiToast::QueueNotification(
      SafeString(data.icon_url), !gameTitle.empty() ? gameTitle : Localize(35264), description,
      TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnAchievementTriggered(const game_rc_achievement_triggered& data)
{
  const std::string title = SafeString(data.title);

  // The add-on carries no timestamp, so the date is "now" - formatted like the
  // dates from the achievement list, so a fresh unlock isn't styled differently.
  const CDateTime unlockedDate = CDateTime::GetCurrentDateTime();

  bool newlyEarned = false;
  CServiceBroker::GetGameServices().AchievementRuntime().MarkEarned(data.id, unlockedDate,
                                                                    newlyEarned);

  // The runtime re-reports achievements that were already earned in an earlier
  // session, so only announce the ones that changed state
  if (!newlyEarned)
  {
    CLog::Log(LOGDEBUG, "CGameClientCheevos: achievement {} \"{}\" was already earned", data.id,
              title);
    return;
  }

  CLog::Log(LOGINFO, "CGameClientCheevos: earned achievement {} \"{}\" ({} points){}", data.id,
            title, data.points, data.hardcore ? " in hardcore mode" : "");

  NotifyDialogs();

  // "Achievement Unlocked" - the one notification that plays a sound
  CGUIDialogKaiToast::QueueNotification(SafeString(data.badge_url), Localize(35281),
                                        !title.empty() ? title : SafeString(data.description),
                                        TOAST_DISPLAY_TIME_MS, true, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnGameCompleted(const std::string& title, bool hardcore)
{
  CLog::Log(LOGINFO, "CGameClientCheevos: {} \"{}\"", hardcore ? "mastered" : "completed", title);

  // "Game mastered" / "Game completed"
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                        Localize(hardcore ? 35282 : 35283), title,
                                        TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnChallengeIndicator(const game_rc_challenge_indicator* indicator)
{
  CAchievementRuntime& runtime = CServiceBroker::GetGameServices().AchievementRuntime();

  ChallengeIndicator attempted;
  if (indicator != nullptr)
  {
    attempted.id = indicator->id;
    attempted.title = indicator->title != nullptr ? indicator->title : "";
    attempted.badgeUrl = indicator->badge_url != nullptr ? indicator->badge_url : "";
  }

  runtime.SetChallengeIndicator(attempted, indicator != nullptr);
  CDialogGameIndicators::Show();
}

void CGameClientCheevos::OnProgressIndicator(const game_rc_progress_indicator* indicator)
{
  ProgressIndicator shown;
  if (indicator != nullptr)
  {
    shown.id = indicator->id;
    shown.title = indicator->title != nullptr ? indicator->title : "";
    shown.badgeUrl = indicator->badge_url != nullptr ? indicator->badge_url : "";
    shown.measuredProgress =
        indicator->measured_progress != nullptr ? indicator->measured_progress : "";
    shown.measuredPercent = indicator->measured_percent;
  }

  CServiceBroker::GetGameServices().AchievementRuntime().SetProgressIndicator(
      shown, indicator != nullptr);
  CDialogGameIndicators::Show();
}

void CGameClientCheevos::OnAchievementProgress(const game_rc_achievement_progress* progress,
                                               unsigned int count)
{
  // Capped like the game-loaded list: the count is the add-on's, and an
  // implausible one would otherwise size an allocation and index the array
  const size_t updateCount = progress != nullptr ? std::min<size_t>(count, MAX_ACHIEVEMENTS) : 0;
  if (updateCount < count)
  {
    CLog::Log(LOGWARNING, "CGameClientCheevos: {} progress updates reported, truncating to {}",
              count, updateCount);
  }

  std::vector<AchievementProgress> updates;
  updates.reserve(updateCount);

  for (size_t i = 0; i < updateCount; ++i)
  {
    AchievementProgress update;
    update.id = progress[i].id;
    update.measuredPercent = progress[i].measured_percent;
    update.measuredProgress = SafeString(progress[i].measured_progress);

    updates.emplace_back(std::move(update));
  }

  const unsigned int applied =
      CServiceBroker::GetGameServices().AchievementRuntime().SetAchievementProgress(updates);
  if (applied != updates.size())
  {
    CLog::Log(LOGWARNING,
              "CGameClientCheevos: {} of {} progress updates were for achievements not in the "
              "loaded game",
              updates.size() - applied, updates.size());
  }

  // No dialog refresh here: progress is deliberately a snapshot taken when the
  // achievements dialog opens, so redrawing the list on every change would
  // both fight the user's scrolling and defeat the point
}

void CGameClientCheevos::OnServerError(const std::string& message, const std::string& api)
{
  CLog::Log(LOGERROR, "CGameClientCheevos: server error from {}: {}",
            !api.empty() ? api : "RetroAchievements", message);

  // "RetroAchievements"
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, Localize(35264), message,
                                        TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnConnectionChanged(bool connected)
{
  // The add-on reports the transition rather than each failed unlock, so this
  // is one notification per outage instead of one per achievement
  CLog::Log(LOGINFO, "CGameClientCheevos: {} RetroAchievements",
            connected ? "reconnected to" : "disconnected from");

  // "Unlocks will be submitted when the connection returns." /
  // "Pending unlocks have been submitted."
  CGUIDialogKaiToast::QueueNotification(
      connected ? CGUIDialogKaiToast::Info : CGUIDialogKaiToast::Warning, Localize(35264),
      Localize(connected ? 35297 : 35296), TOAST_DISPLAY_TIME_MS, false, TOAST_MESSAGE_TIME_MS);
}

void CGameClientCheevos::OnRichPresenceUpdated(const std::string& evaluation)
{
  CServiceBroker::GetGameServices().AchievementRuntime().SetRichPresence(evaluation);
}

void CGameClientCheevos::OnLoginResult(const game_rc_login_result& data)
{
  if (data.success)
  {
    const std::string username = !SafeString(data.display_name).empty()
                                     ? SafeString(data.display_name)
                                     : SafeString(data.username);

    // Not announced to the player: the add-on signs in on every game load, and
    // the sign-in performed from Settings reports its own result
    CLog::Log(LOGINFO, "CGameClientCheevos: logged in as \"{}\" with {} points", username,
              data.points);
  }
  else
  {
    CLog::Log(LOGWARNING, "CGameClientCheevos: login failed: {}",
              !SafeString(data.error_message).empty() ? SafeString(data.error_message)
                                                      : "no reason given");
  }

  // Keep Kodi's logged-in state in step with the add-on, so that a rejected
  // token doesn't leave the UI claiming the player is logged in.
  //
  // Only a rejection clears it. Clearing on every failure would have a server
  // outage erase the saved token -- CGameSettings::OnSettingChanged() drops it
  // whenever this goes false -- signing the player out for good and leaving
  // the add-on's reconnect with nothing to retry.
  if (data.success)
    CServiceBroker::GetGameServices().GameSettings().SetAchievementsLoggedIn(true);
  else if (data.credentials_rejected)
    CServiceBroker::GetGameServices().GameSettings().SetAchievementsLoggedIn(false);
}

void CGameClientCheevos::OnGameClosed()
{
  CServiceBroker::GetGameServices().AchievementRuntime().Clear();

  NotifyDialogs();
}

bool CGameClientCheevos::SendCredentials()
{
  const CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  const std::string username = gameSettings.GetRAUsername();
  const std::string token = gameSettings.GetRAToken();

  // Sent even when there are none. The client keeps its session across games
  // on the same instance, so a player who signed out between them would
  // otherwise keep submitting unlocks under the account they just left. Empty
  // credentials are the signal to drop it.
  //
  // The token is what the add-on signs in with; the password never leaves Kodi
  if (username.empty() || token.empty())
    return m_gameClient.SetRetroAchievementsCredentials("", "");

  return m_gameClient.SetRetroAchievementsCredentials(username, token);
}
