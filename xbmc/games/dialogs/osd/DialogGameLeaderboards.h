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

#include <set>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief The leaderboards of the game being played
 *
 * A leaderboard is a scored challenge - a fastest lap, a highest score on one
 * level - and a game may have dozens. The list shows what the game offers,
 * where the player stands on each, and who holds the top place; choosing one
 * opens its standings.
 *
 * The definitions come from the same RetroAchievements response the
 * achievements do, so opening this costs nothing. The standings do not: each
 * leaderboard is a separate request, so they are fetched in the background
 * and the rows fill in as answers arrive.
 *
 * Browsing only. Submitting a score requires hardcore mode, which
 * RetroAchievements only grants an emulator once it has been approved.
 *
 * \section leaderboard_properties What a skin is given
 *
 * On the window, \c Leaderboards.Status, set while loading or when the game
 * has none. A skin shows the list only when it is empty.
 *
 * On each row:
 *
 *  - \c ListItem.Label     The leaderboard's name
 *  - \c ListItem.Label2    What it measures
 *  - \c LeaderboardId      Its RetroAchievements id
 *  - \c Format             How the score reads - "Time, lowest wins"
 *  - \c TopUsername        Who holds first place, once the standings arrive
 *  - \c TopScore           Their score
 *  - \c TotalEntries       How many have set one
 *  - \c PlayerRank         Where the player stands, or the text for unranked
 *  - \c PlayerScore        Their score, if they have one
 *
 * Everything from \c TopUsername down arrives later than the row does: the
 * definitions come free with the achievements, the standings are a request
 * each. A skin should expect those to be empty at first and fill in.
 */
class CDialogGameLeaderboards : public CGUIDialog, protected CJobQueue
{
public:
  //! How many standings to ask for when the dialog opens. The rest are fetched
  //! as the player scrolls to them.
  static constexpr unsigned int STANDINGS_PREFETCH = 8;

public:
  CDialogGameLeaderboards();
  ~CDialogGameLeaderboards() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;

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
  //! Build the list from whatever the runtime currently holds
  void PopulateList();

  //! Ask RetroAchievements which leaderboards this game has
  void FetchList(unsigned int gameId);

  //! Ask for the standings of the first few, so the list has something in it
  void FetchStandings();

  //! Ask for one leaderboard's standings, as the player reaches it
  void FetchStandingsFor(unsigned int leaderboardId);

  //! A one-line summary of what a leaderboard measures and which way it ranks
  static std::string DescribeFormat(const std::string& format, bool lowerIsBetter);

  CGUIViewControl m_viewControl;
  CFileItemList m_items;

  //! Guards dialog state shared with the job thread
  CCriticalSection m_section;

  //! Where the player was, so reopening after looking at one leaderboard does
  //! not drop them back at the top of a long list
  int m_lastSelected{-1};

  //! Leaderboards already asked about, so scrolling up and down a list does
  //! not ask again for every row it passes over
  std::set<unsigned int> m_requested;

  //! The row whose standings were last asked for, so the fetch happens once
  //! per row rather than once per frame
  int m_lastFetched{-1};
};

} // namespace GAME
} // namespace KODI
