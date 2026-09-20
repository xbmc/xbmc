/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
struct LeaderboardEntry;

/*!
 * \ingroup games
 *
 * \brief Turn a raw leaderboard value into something readable
 *
 * RetroAchievements sends scores as plain integers and says separately how to
 * read them: 9000 is nine thousand points on one leaderboard and two and a half
 * minutes on another. Both leaderboard dialogs need this.
 *
 * Follows rcheevos, which is what formats the same game's live tracker inside
 * the add-on. The two would otherwise write one leaderboard's values two ways.
 *
 * \param score The value as sent
 * \param format The leaderboard's format, as the server names it. Anything
 *        unrecognised is shown as a plain number, since a wrong unit reads
 *        worse than none.
 */
/*!
 * \ingroup games
 *
 * \brief Whether a leaderboard measures a duration rather than a quantity
 *
 * Every format FormatLeaderboardScore() reads as a time is named here, so that
 * what a value looks like and what the board is called cannot disagree.
 */
bool IsTimeFormat(const std::string& format);

std::string FormatLeaderboardScore(int score, const std::string& format);

/*!
 * \ingroup games
 *
 * \brief How long ago something was submitted, in words
 *
 * A leaderboard is read down a column, and "3 months ago" is taken in at a
 * glance where a date has to be worked out against today's. The exact date is
 * still published alongside for anyone who wants it.
 *
 * \param submitted When it was submitted, as a unix timestamp
 * \param now The current time, passed in so the result can be tested
 *
 * \return The age in words, or an empty string if the timestamp is unusable
 */
std::string FormatRelativeDate(std::time_t submitted, std::time_t now);

/*!
 * \ingroup games
 *
 * \brief Which medal a place on a leaderboard earns, if any
 *
 * \return "gold", "silver", "bronze", or an empty string below third. Given to
 *         the skin as a property so it can colour the rank however it likes,
 *         rather than being drawn here.
 */
std::string RankMedal(unsigned int rank);

/*!
 * \ingroup games
 *
 * \brief Remember a leaderboard's standings between sessions
 *
 * Standings are a request each and change slowly - a table that has stood for
 * months will not have moved because Kodi restarted. Keeping them means
 * reopening a leaderboard already looked at costs nothing, and a game with
 * thirty of them is not thirty requests every time.
 *
 * Written to userdata rather than a database: there are few of them, they are
 * small, and losing them costs one refetch.
 *
 * \param leaderboardId The leaderboard
 * \param account The RetroAchievements account the standings were read for
 * \param entries Its standings, as fetched
 */
void SaveLeaderboardEntries(unsigned int leaderboardId,
                            const std::string& account,
                            const std::vector<LeaderboardEntry>& entries);

/*!
 * \ingroup games
 *
 * \brief Read back standings kept from a previous session
 *
 * \param leaderboardId The leaderboard
 * \param account The RetroAchievements account the standings were read for
 * \param[out] entries Filled in if anything was kept and it is still fresh
 *
 * \return False if nothing was kept, or what was kept has gone stale
 */
bool LoadLeaderboardEntries(unsigned int leaderboardId,
                            const std::string& account,
                            std::vector<LeaderboardEntry>& entries);

/*!
 * \ingroup games
 *
 * \brief Forget a leaderboard's kept standings
 *
 * Called when the player's own standing on it changes. The kept page was
 * fetched before they submitted, so it is wrong rather than merely old, and
 * leaving it would show them a table they are no longer in the right place in
 * until it aged out a week later.
 */
void ForgetLeaderboardEntries(unsigned int leaderboardId);

/*!
 * \ingroup games
 *
 * \brief Forget every leaderboard's kept standings
 *
 * Offered to the player because the kept copies are the only thing between them
 * and a fresh answer, and a table that has been beaten in the last few minutes
 * is otherwise a week away from being asked for again.
 */
void ClearLeaderboardEntries();

} // namespace GAME
} // namespace KODI
