/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/SetInfoTag.h"

#include <string>

#include <gtest/gtest.h>

TEST(TestSetInfoTag, NfoVersionZero)
{
  CXBMCTinyXML input;
  const std::string document =
      "<set><title>Test set</title><originaltitle>Original title</originaltitle>"
      "<overview>Test overview</overview><art><poster>poster.jpg</poster></art></set>";
  input.Parse(document);
  CSetInfoTag details;
  ASSERT_TRUE(details.Load(input.RootElement()));
  EXPECT_EQ("Test set", details.GetTitle());
  EXPECT_EQ("Original title", details.GetOriginalTitle());
  EXPECT_EQ("Test overview", details.GetOverview());
  EXPECT_EQ("poster.jpg", details.GetArt().at("poster"));

  CXBMCTinyXML saved;
  ASSERT_TRUE(details.Save(&saved, "set"));
  ASSERT_NE(nullptr, saved.RootElement());
  EXPECT_STREQ("0", saved.RootElement()->Attribute("version"));
  int version = -1;
  EXPECT_EQ(TIXML_SUCCESS, saved.RootElement()->QueryIntAttribute("version", &version));
  EXPECT_EQ(0, version);

  CSetInfoTag reloaded;
  ASSERT_TRUE(reloaded.Load(saved.RootElement()));
  TiXmlElement container("videodb");
  ASSERT_TRUE(reloaded.Save(&container, "set"));
  EXPECT_EQ(nullptr, container.Attribute("version"));
  EXPECT_TRUE(
      XMLUtils::AreNodesSerializationsEqual(saved.RootElement(), container.FirstChildElement()));

  saved.RootElement()->RemoveAttribute("version");
  EXPECT_TRUE(XMLUtils::AreNodesSerializationsEqual(input.RootElement(), saved.RootElement()));
  CXBMCTinyXML unrelated;
  ASSERT_TRUE(details.Save(&unrelated, "details"));
  EXPECT_EQ(nullptr, unrelated.RootElement()->Attribute("version"));
}
