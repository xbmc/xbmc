/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemList.h"
#include "guilib/GUIDialog.h"
#include "jobs/JobQueue.h"
#include "threads/CriticalSection.h"
#include "view/GUIViewControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief The standings of one leaderboard
 *
 * Reached from the leaderboards list, which records which one was chosen on
 * the achievement runtime - the skin opens this window and has no way to pass
 * anything with it.
 *
 * The player's own row is marked so a skin can pick it out, and is fetched
 * along with the top of the table even when they are far enough down it not to
 * appear: seeing where you stand is the point of looking.
 *
 * \section leaderboard_entry_properties What a skin is given
 *
 * On the window:
 *
 *  - \c Leaderboards.Title       The leaderboard's name
 *  - \c Leaderboards.Description What it measures
 *  - \c Leaderboards.Status      Set while loading, or when there is nothing
 *                                to show. A skin shows the list only when this
 *                                is empty.
 *  - \c Leaderboards.PlayerBest  Where the player stands, as a whole sentence,
 *                                including the case where they have never set
 *                                a time. Empty if not signed in.
 *
 * On each row:
 *
 *  - \c ListItem.Label           The username
 *  - \c ListItem.Label2          The score, already in the leaderboard's units
 *  - \c ListItem.Icon            Their RetroAchievements avatar
 *  - \c Rank / \c RankLabel      The place, as a number and as text
 *  - \c Medal                    "gold", "silver", "bronze", or empty. The
 *                                colour is the skin's decision; nothing is
 *                                drawn here.
 *  - \c Date                     The submission date in the player's locale
 *  - \c DateRelative             The same, in words - "3 months ago"
 *  - \c IsPlayer                 Non-empty on the signed-in player's own row
 */
class CDialogGameLeaderboardEntries : public CGUIDialog, protected CJobQueue
{
public:
  CDialogGameLeaderboardEntries();
  ~CDialogGameLeaderboardEntries() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  // Implementation of IJobCallback via CJobQueue
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

private:
  //! Build the list from whatever the runtime holds for this leaderboard
  void PopulateList();

  /*!
   * \brief Ask RetroAchievements for this leaderboard's standings
   *
   * Does nothing if they are already held.
   */
  void FetchEntries();

  //! Publish the heading and status for the skin
  void SetStatus(const std::string& status);

  CGUIViewControl m_viewControl;
  CFileItemList m_items;

  //! Guards the list against the job thread
  CCriticalSection m_section;

  //! The leaderboard being shown
  unsigned int m_leaderboardId{0};
};

} // namespace GAME
} // namespace KODI
