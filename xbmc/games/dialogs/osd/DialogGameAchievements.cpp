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
#include "dialogs/GUIDialogKaiToast.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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
  if (!gameSettings.GetAchievementsLoggedIn())
  {
    // Opened rather than refused: someone without an account has no other way
    // to find out the feature is there. The skin shows them how to get one.
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

void CDialogGameAchievements::RefreshList()
{
  const AchievementState state = CServiceBroker::GetGameServices().AchievementRuntime().GetState();

  // Remembered by achievement rather than by row: an unlock moves its
  // achievement from the locked group into the earned one, so the row that
  // was selected is not the row it ends up on
  const int previousItem = m_viewControl->GetSelectedItem();
  unsigned int selectedId = 0;
  if (previousItem >= 0 && previousItem < m_items->Size())
    selectedId = (*m_items)[previousItem]->GetProperty(ACHIEVEMENT_ID).asUnsignedInteger();

  m_viewControl->Clear();
  m_items->Clear();

  std::vector<AchievementInfo> achievements = state.achievements;

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
      item->SetArt("icon", badgeUrl);

    // Not shown; carried so the selection survives a resort
    item->SetProperty(ACHIEVEMENT_ID, achievement.id);

    // "{0:d} pts"
    item->SetProperty(ACHIEVEMENT_POINTS, StringUtils::Format(Localize(35294), achievement.points));
    // Only set when true: a CVariant holding false stringifies to "false",
    // which String.IsEmpty() in the skin reads as present
    if (achievement.earned)
      item->SetProperty(ACHIEVEMENT_EARNED, true);

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
  std::string header = Localize(35287);
  if (!state.gameTitle.empty())
    header += " - " + state.gameTitle;
  if (!progress.empty())
    header += " (" + progress + ")";

  SetProperty("Header", header);
}
