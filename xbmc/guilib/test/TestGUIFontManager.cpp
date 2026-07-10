/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIFontManager.h"

#include <string>

#include <gtest/gtest.h>

TEST(TestGUIFontManager, MakeFontIdentDistinguishesSameBasenameDifferentPath)
{
  // Two addons each ship "MyFont.ttf" at the same size. Keyed on the basename
  // they collide in the shared CGUIFontTTF pool and the second renders the
  // first one's glyphs.
  const std::string a =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string b =
      GUIFontManager::MakeFontIdent("/addons/b/fonts/MyFont.ttf", 24.0f, 1.0f, false);

  EXPECT_NE(a, b);
}

TEST(TestGUIFontManager, MakeFontIdentSharesIdentForIdenticalPathSizeAspect)
{
  const std::string a =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string b =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);

  EXPECT_EQ(a, b);
}

TEST(TestGUIFontManager, MakeFontIdentDistinguishesBorder)
{
  const std::string plain = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string border = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, true);

  EXPECT_NE(plain, border);
}

TEST(TestGUIFontManager, MakeFontIdentDistinguishesSizeAndAspect)
{
  const std::string s24 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string s30 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 30.0f, 1.0f, false);
  const std::string a15 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.5f, false);

  EXPECT_NE(s24, s30);
  EXPECT_NE(s24, a15);
}
