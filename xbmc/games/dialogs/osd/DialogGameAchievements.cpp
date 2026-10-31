/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameAchievements.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int TOAST_DISPLAY_TIME_MS = 6000;
constexpr unsigned int TOAST_MESSAGE_TIME_MS = 500;

// Rarity thresholds, as a percentage of players who have earned it
constexpr float RARITY_COMMON = 50.0f;
constexpr float RARITY_UNCOMMON = 10.0f;
constexpr float RARITY_RARE = 2.0f;

std::string Localize(uint32_t stringId)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(stringId);
}

/*!
 * \brief Translate a rarity percentage into a localized category
 *
 * \return The category, or an empty string if the rarity is unknown
 */
std::string RarityCategory(float rarity)
{
  if (rarity <= 0.0f)
    return {};

  if (rarity > RARITY_COMMON)
    return Localize(35290); // "Common"
  if (rarity > RARITY_UNCOMMON)
    return Localize(35291); // "Uncommon"
  if (rarity > RARITY_RARE)
    return Localize(35292); // "Rare"

  return Localize(35293); // "Ultra rare"
}

/*!
 * \brief Render a rarity percentage as one to four stars
 *
 * Kept apart from the category name so that the name stays translatable and
 * the stars stay out of the translated strings.
 *
 * \return The stars, or an empty string if the rarity is unknown
 */
std::string RarityStars(float rarity)
{
  if (rarity <= 0.0f)
    return {};

  if (rarity > RARITY_COMMON)
    return "★";
  if (rarity > RARITY_UNCOMMON)
    return "★★";
  if (rarity > RARITY_RARE)
    return "★★★";

  return "★★★★";
}
} // namespace

CDialogGameAchievements::CDialogGameAchievements()
  : CGUIDialog(WINDOW_DIALOG_GAME_ACHIEVEMENTS, "DialogGameControllers.xml"),
    m_items(std::make_unique<CFileItemList>()),
    m_viewControl(std::make_unique<CGUIViewControl>())
{
}

CDialogGameAchievements::~CDialogGameAchievements() = default;

void CDialogGameAchievements::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl->Reset();
  m_viewControl->SetParentWindow(GetID());
  m_viewControl->AddView(GetControl(CONTROL_CHEEVOS_LIST));
}

void CDialogGameAchievements::OnWindowUnload()
{
  m_viewControl->Reset();

  CGUIDialog::OnWindowUnload();
}

void CDialogGameAchievements::OnInitWindow()
{
  const CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  const auto appPlayer = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  if (!gameSettings.GetAchievementsLoggedIn() || !appPlayer->IsPlayingGame())
  {
    CGUIDialog::OnInitWindow();
    return;
  }

  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();

  // Identification is a network round trip, so a game opened moments ago has
  // not been answered for yet. Saying it has no achievements would be a guess,
  // and usually the wrong one.
  if (!state.loaded)
  {
    // "RetroAchievements", "Still looking this game up on RetroAchievements..."
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Localize(35264),
                                          Localize(35299), TOAST_DISPLAY_TIME_MS, false,
                                          TOAST_MESSAGE_TIME_MS);
    Abort();
    return;
  }

  if (state.achievements.empty())
  {
    // "RetroAchievements", "This game doesn't support RetroAchievements"
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Localize(35264),
                                          Localize(35286), TOAST_DISPLAY_TIME_MS, false,
                                          TOAST_MESSAGE_TIME_MS);
    Abort();
    return;
  }

  // Before the base class, which focuses the list and can't focus an empty one
  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);
  RefreshList();

  CGUIDialog::OnInitWindow();
}

void CDialogGameAchievements::Abort()
{
  // Forced: Open() makes the dialog active before OnInitWindow() runs, so a
  // plain Close() would animate out and flash
  Close(true);
}

void CDialogGameAchievements::OnDeinitWindow(int nextWindowID)
{
  m_viewControl->Clear();
  m_items->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CDialogGameAchievements::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int control = message.GetSenderId();
      if (control == CONTROL_CHEEVOS_HARDCORE)
      {
        OnHardcoreToggled();
        return true;
      }
      if (control == CONTROL_CHEEVOS_SET)
      {
        SelectSet(1);
        return true;
      }
      if (control == CONTROL_CHEEVOS_SET_ENABLED)
      {
        OnSetToggled();
        return true;
      }
      if (control == CONTROL_CHEEVOS_ENCORE || control == CONTROL_CHEEVOS_CHALLENGE_INDICATOR)
      {
        const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
        settings->ToggleBool(control == CONTROL_CHEEVOS_ENCORE
                                 ? "gamesachievements.encore"
                                 : "gamesachievements.challengeindicator");
        settings->Save();
        return true;
      }
      break;
    }
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_REFRESH_LIST)
      {
        // A targeted thread message reaches this window whether or not it is
        // open, and every unlock sends one. Rebuilding would sort the whole
        // set and construct a CFileItem per achievement on the GUI thread
        // during ordinary play. OnInitWindow() rebuilds when it is opened.
        if (IsActive())
          RefreshList();
        return true;
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CDialogGameAchievements::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_NEXT_ITEM:
      SelectSet(1);
      return true;
    case ACTION_PREV_ITEM:
      SelectSet(-1);
      return true;
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CDialogGameAchievements::SelectSet(int step)
{
  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();
  if (state.sets.size() < 2)
    return;

  const auto current = std::ranges::find(state.sets, m_setId, &AchievementSet::id);
  const int count = static_cast<int>(state.sets.size());
  const int index = current != state.sets.end()
                        ? static_cast<int>(std::distance(state.sets.begin(), current))
                        : 0;

  m_setId = state.sets[((index + step) % count + count) % count].id;

  // A different set's rows, so the old selection means nothing
  m_viewControl->SetSelectedItem(0);
  RefreshList();
}

void CDialogGameAchievements::OnSetToggled()
{
  CAchievementRuntime& runtime = CServiceBroker::GetGameServices().AchievementRuntime();
  const AchievementState state = runtime.GetState();

  // The list refreshes itself once the client has done it
  if (!runtime.RequestAchievementSetEnabled(m_setId, !state.IsSetEnabled(m_setId)))
    RefreshList();
}

void CDialogGameAchievements::RefreshList()
{
  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();

  // The game's own set until another is picked, and again if the one picked
  // went away with the game
  if (std::ranges::find(state.sets, m_setId, &AchievementSet::id) == state.sets.end())
    m_setId = state.sets.empty() ? 0 : state.sets.front().id;
  const auto shown = std::ranges::find(state.sets, m_setId, &AchievementSet::id);
  const AchievementSet* shownSet = shown != state.sets.end() ? &*shown : nullptr;

  // Remembered by achievement rather than by row: an unlock moves its
  // achievement from the locked group into the earned one, so the row that
  // was selected is not the row it ends up on
  const int previousItem = m_viewControl->GetSelectedItem();
  unsigned int selectedId = 0;
  if (previousItem >= 0 && previousItem < m_items->Size())
    selectedId = (*m_items)[previousItem]->GetProperty(ACHIEVEMENT_ID).asUnsignedInteger();

  m_viewControl->Clear();
  m_items->Clear();

  std::vector<AchievementInfo> achievements;
  std::ranges::copy_if(state.achievements, std::back_inserter(achievements),
                       [this](const AchievementInfo& achievement)
                       { return m_setId == 0 || achievement.setId == m_setId; });

  // Earned first; the runtime's order is kept within each group so the list
  // doesn't reshuffle as achievements unlock
  std::stable_sort(achievements.begin(), achievements.end(),
                   [](const AchievementInfo& lhs, const AchievementInfo& rhs)
                   { return lhs.earned && !rhs.earned; });

  uint64_t totalPoints = 0;
  uint64_t earnedPoints = 0;

  for (const AchievementInfo& achievement : achievements)
  {
    auto item = std::make_shared<CFileItem>(achievement.title);
    item->SetLabel(achievement.title);
    item->SetLabel2(achievement.description);

    const std::string& badgeUrl = (achievement.earned || achievement.lockedBadgeUrl.empty())
                                      ? achievement.badgeUrl
                                      : achievement.lockedBadgeUrl;
    if (!badgeUrl.empty())
      item->SetArt(ART::TYPE::ICON, badgeUrl);

    // Not shown; carried so the selection survives a resort
    item->SetProperty(ACHIEVEMENT_ID, achievement.id);

    // "{0:d} pts"
    item->SetProperty(ACHIEVEMENT_POINTS, StringUtils::Format(Localize(35294), achievement.points));
    // Only set when true: a CVariant holding false stringifies to "false",
    // which String.IsEmpty() in the skin reads as present
    if (achievement.earned)
      item->SetProperty(ACHIEVEMENT_EARNED, true);
    else if (achievement.type == AchievementType::MISSABLE)
      item->SetProperty(ACHIEVEMENT_MISSABLE, true);

    if (achievement.earnedHardcore)
      item->SetProperty(ACHIEVEMENT_HARDCORE, true);

    // Earned or not, so the player can see what beating the game took
    if (achievement.type == AchievementType::PROGRESSION)
      item->SetProperty(ACHIEVEMENT_PROGRESSION, true);
    else if (achievement.type == AchievementType::WIN)
      item->SetProperty(ACHIEVEMENT_WIN, true);

    if (achievement.earned && achievement.unlockedDate.IsValid())
    {
      // "Unlocked {0:s}"
      item->SetProperty(
          ACHIEVEMENT_UNLOCKED_DATE,
          StringUtils::Format(Localize(35289), achievement.unlockedDate.GetAsLocalizedDate(
                                                   std::string{"MMM dd yyyy"})));
    }

    if (achievement.rarity > 0.0f)
    {
      item->SetProperty(ACHIEVEMENT_RARITY_CATEGORY, RarityCategory(achievement.rarity));
      item->SetProperty(ACHIEVEMENT_RARITY_STARS, RarityStars(achievement.rarity));
    }

    // An earned achievement is by definition finished
    const bool measured = !achievement.earned && !achievement.measuredProgress.empty();
    if (measured)
    {
      item->SetProperty(ACHIEVEMENT_MEASURED, true);
      item->SetProperty(ACHIEVEMENT_MEASURED_PROGRESS, achievement.measuredProgress);
      if (achievement.measuredPercent == 0.0f)
        item->SetProperty(ACHIEVEMENT_MEASURED_ZERO, true);
    }

    // Set on every row: a list layout shares one progress control, which keeps
    // its last percentage when an item doesn't resolve the info, so a row
    // without this would draw the previous row's bar. Integer for asInteger().
    item->SetProperty(ACHIEVEMENT_MEASURED_PERCENT,
                      measured ? static_cast<int>(std::lround(
                                     std::clamp(achievement.measuredPercent, 0.0f, 100.0f)))
                               : 0);

    totalPoints += achievement.points;
    if (achievement.earned)
      earnedPoints += achievement.points;

    m_items->Add(std::move(item));
  }

  m_viewControl->SetItems(*m_items);

  // std::clamp() is undefined when the list is empty, since the upper bound
  // would fall below the lower one
  if (!m_items->IsEmpty())
  {
    int selectedItem = previousItem;
    if (selectedId != 0)
    {
      for (int i = 0; i < m_items->Size(); ++i)
      {
        if ((*m_items)[i]->GetProperty(ACHIEVEMENT_ID).asUnsignedInteger() == selectedId)
        {
          selectedItem = i;
          break;
        }
      }
    }

    m_viewControl->SetSelectedItem(std::clamp(selectedItem, 0, m_items->Size() - 1));
  }

  // Progress weighted by point value, which is how RetroAchievements measures
  // completion. Guard against a game whose achievements are all worth 0 points.
  std::string progress;
  if (totalPoints > 0)
  {
    const int percent = static_cast<int>((earnedPoints * 100) / totalPoints);

    // "{0:d}% complete"
    progress = StringUtils::Format(Localize(35288), percent);
  }

  // Build the header here rather than in the skin, so that skins don't have to
  // reproduce the punctuation and the empty cases
  //
  // "Achievements - Chrono Trigger (74% complete)"
  // "Main Set" for the game's own set, which RetroAchievements leaves untitled
  std::string setTitle = state.gameTitle;
  if (shownSet != nullptr && shownSet == &state.sets.front())
    setTitle = Localize(35715);
  else if (shownSet != nullptr && !shownSet->title.empty())
    setTitle = shownSet->title;
  const std::string title = state.sets.size() > 1 ? setTitle : state.gameTitle;

  const bool enabled = shownSet == nullptr || shownSet->enabled;
  const bool toggleable = shownSet != nullptr && shownSet != &state.sets.front();

  // "Turned off"
  if (!enabled)
    progress = Localize(35714);

  std::string header = Localize(35287);
  if (!title.empty())
    header += " - " + title;
  if (!progress.empty())
    header += " (" + progress + ")";

  // "Beaten" or "{0:d} of {1:d} to beat"
  std::string beaten;
  const BeatenProgress beatenProgress = state.GetBeatenProgress(m_setId);
  if (beatenProgress.IsBeaten())
  {
    beaten = Localize(35712);
  }
  else if (beatenProgress.CanBeBeaten())
  {
    const unsigned int needed = beatenProgress.progression + (beatenProgress.win > 0 ? 1 : 0);
    const unsigned int earned =
        beatenProgress.progressionEarned + (beatenProgress.winEarned > 0 ? 1 : 0);
    beaten = StringUtils::Format(Localize(35713), earned, needed);
  }

  SetProperty("Header", header);
  SetProperty("Achievements.GameTitle", title);
  SetProperty("Achievements.Progress", progress);
  SetProperty("Achievements.Beaten", enabled ? beaten : "");
  SetProperty("Achievements.HasSets", state.sets.size() > 1 ? "true" : "");
  SetProperty("Achievements.SetTitle", setTitle);
  SetProperty("Achievements.SetEnabled", enabled ? "true" : "");
  SetProperty("Achievements.SetToggleable", toggleable ? "true" : "");
}

void CDialogGameAchievements::OnHardcoreToggled()
{
  CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();

  const bool enabling = !gameSettings.GetAchievementsHardcore();

  // Turning it on during a game asks first, and the radio button takes its
  // state from the setting, so a refusal leaves both as they were
  gameSettings.SetAchievementsHardcore(enabling);
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  // The whole OSD goes, not just this dialog: the player asked for the game to
  // restart, and leaving them on the menu they opened hides it. An emulator
  // that refused hardcore doesn't restart.
  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  if (enabling && gameClient && gameClient->Cheevos().IsHardcoreActive())
    CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog()->CloseOSD();
}
