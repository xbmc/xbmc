/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "games/tags/GameInfoTag.h"
#include "utils/SortUtils.h"

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
void AddGames(CFileItemList& items)
{
  struct Game
  {
    std::string path;
    std::string title;
    unsigned int year;
    std::string genre;
    float rating;
  };

  // Listed by file name, which sorts differently to every detail
  const std::vector<Game> games{{"/roms/1.rom", "Secret of Mana", 1993, "Role-playing", 8.5f},
                                {"/roms/2.rom", "Actraiser", 1990, "Simulation", 7.9f},
                                {"/roms/3.rom", "Chrono Trigger", 1995, "Adventure", 9.4f}};

  for (const Game& game : games)
  {
    auto item = std::make_shared<CFileItem>(game.path, false);
    item->SetLabel(game.path);
    CGameInfoTag* tag = item->GetGameInfoTag();
    tag->SetTitle(game.title);
    tag->SetYear(game.year);
    tag->SetGenres({game.genre});
    tag->SetRating(game.rating);
    items.Add(std::move(item));
  }
}

std::vector<std::string> Titles(const CFileItemList& items)
{
  std::vector<std::string> titles;
  for (const auto& item : items)
    titles.emplace_back(item->GetGameInfoTag()->GetTitle());
  return titles;
}
} // namespace

TEST(TestGameInfoTag, SortsByTitle)
{
  CFileItemList items;
  AddGames(items);
  items.Sort(SortBy::TITLE, SortOrder::ASCENDING);
  EXPECT_EQ(Titles(items),
            (std::vector<std::string>{"Actraiser", "Chrono Trigger", "Secret of Mana"}));
}

TEST(TestGameInfoTag, SortsByYear)
{
  CFileItemList items;
  AddGames(items);
  items.Sort(SortBy::YEAR, SortOrder::ASCENDING);
  EXPECT_EQ(Titles(items),
            (std::vector<std::string>{"Actraiser", "Secret of Mana", "Chrono Trigger"}));
}

TEST(TestGameInfoTag, SortsByGenre)
{
  CFileItemList items;
  AddGames(items);
  items.Sort(SortBy::GENRE, SortOrder::ASCENDING);
  EXPECT_EQ(Titles(items),
            (std::vector<std::string>{"Chrono Trigger", "Secret of Mana", "Actraiser"}));
}

TEST(TestGameInfoTag, SortsByRating)
{
  CFileItemList items;
  AddGames(items);
  items.Sort(SortBy::RATING, SortOrder::ASCENDING);
  EXPECT_EQ(Titles(items),
            (std::vector<std::string>{"Actraiser", "Secret of Mana", "Chrono Trigger"}));
}

TEST(TestGameInfoTag, ComparesTheWiderDetails)
{
  CGameInfoTag tag;
  tag.SetLoaded();
  tag.SetTags({"JRPG"});

  CGameInfoTag copy{tag};
  EXPECT_EQ(copy, tag);

  copy.SetPlayers(2);
  EXPECT_NE(copy, tag);
}

TEST(TestGameInfoTag, IgnoresARatingOutsideZeroToTen)
{
  CGameInfoTag tag;
  tag.SetRating(8.5f);

  tag.SetRating(11.0f);
  tag.SetRating(-1.0f);
  tag.SetRating(std::numeric_limits<float>::quiet_NaN());
  EXPECT_EQ(tag.GetRating(), 8.5f);

  tag.SetRating(0.0f);
  EXPECT_EQ(tag.GetRating(), 0.0f);
}
