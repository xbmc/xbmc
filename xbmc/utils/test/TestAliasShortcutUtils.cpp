/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/AliasShortcutUtils.h"

#include "gtest/gtest.h"

TEST(TestAliasShortcutUtils, IsAliasShortcut)
{
  CStdString a = "";
#if defined(TARGET_DARWIN_OSX)
  /* TODO: Write test case for OSX */
#else
  EXPECT_FALSE(IsAliasShortcut(a));
#endif
}

TEST(TestAliasShortcutUtils, TranslateAliasShortcut)
{
  CStdString a = "";
  TranslateAliasShortcut(a);
#if defined(TARGET_DARWIN_OSX)
  /* TODO: Write test case for OSX */
#else
  EXPECT_STREQ("", a.c_str());
#endif
}
