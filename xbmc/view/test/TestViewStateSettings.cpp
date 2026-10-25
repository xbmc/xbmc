/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "view/ViewState.h"
#include "view/ViewStateNames.h"
#include "view/ViewStateSettings.h"

#include <string>

#include <gtest/gtest.h>

namespace
{
std::string Entry(const std::string& name, int viewMode, SortBy sortBy)
{
  return StringUtils::Format("<{0}><viewmode>{1}</viewmode><sortmethod>{2}</sortmethod>"
                             "<sortorder>1</sortorder><sortattributes>0</sortattributes></{0}>",
                             name, viewMode, static_cast<int>(sortBy));
}

class TestViewStateSettings : public ::testing::Test
{
protected:
  void SetUp() override { CViewStateSettings::GetInstance().Save(&m_saved); }
  void TearDown() override { CViewStateSettings::GetInstance().Load(&m_saved); }

  const CViewState* LoadGames(const std::string& viewStates)
  {
    CXBMCTinyXML doc;
    doc.Parse("<settings><viewstates>" + viewStates + "</viewstates></settings>");
    EXPECT_TRUE(CViewStateSettings::GetInstance().Load(doc.RootElement()));
    return CViewStateSettings::GetInstance().Get(KODI::VIEW::STATE::GAMES);
  }

  TiXmlElement m_saved{"settings"};
};
} // namespace

TEST_F(TestViewStateSettings, GamesKeepsFileWhereLabelWasOnlyTheDefault)
{
  const CViewState* games = LoadGames(Entry("games", DEFAULT_VIEW_ICONS, SortBy::LABEL));
  ASSERT_NE(games, nullptr);
  EXPECT_EQ(games->m_viewMode, DEFAULT_VIEW_ICONS);
  EXPECT_EQ(games->m_sortDescription.sortBy, SortBy::FILE);
}

TEST_F(TestViewStateSettings, GamesKeepsASortChosenBefore)
{
  const CViewState* games = LoadGames(Entry("games", DEFAULT_VIEW_ICONS, SortBy::SIZE));
  ASSERT_NE(games, nullptr);
  EXPECT_EQ(games->m_sortDescription.sortBy, SortBy::SIZE);
}

TEST_F(TestViewStateSettings, GamesKeepsNameOnceChosen)
{
  const CViewState* games =
      LoadGames(Entry("games", DEFAULT_VIEW_ICONS, SortBy::SIZE) +
                Entry(KODI::VIEW::STATE::GAMES, DEFAULT_VIEW_LIST, SortBy::LABEL));
  ASSERT_NE(games, nullptr);
  EXPECT_EQ(games->m_viewMode, DEFAULT_VIEW_LIST);
  EXPECT_EQ(games->m_sortDescription.sortBy, SortBy::LABEL);
}
