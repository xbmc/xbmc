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
  };

  // Listed by file name, which sorts differently to every detail
  const std::vector<Game> games{{"/roms/1.rom", "Secret of Mana", 1993, "Role-playing"},
                                {"/roms/2.rom", "Actraiser", 1990, "Simulation"},
                                {"/roms/3.rom", "Chrono Trigger", 1995, "Adventure"}};

  for (const Game& game : games)
  {
    auto item = std::make_shared<CFileItem>(game.path, false);
    item->SetLabel(game.path);
    CGameInfoTag* tag = item->GetGameInfoTag();
    tag->SetTitle(game.title);
    tag->SetYear(game.year);
    tag->SetGenres({game.genre});
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
