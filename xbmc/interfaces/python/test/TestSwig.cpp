/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "../swig.h"

#include <gtest/gtest.h>

using namespace PythonBindings;

TEST(TestSwig, TypeConversion)
{
  EXPECT_TRUE(isParameterRightType("p.XBMCAddon::xbmcgui::ListItem","p.XBMCAddon::xbmcgui::ListItem","XBMCAddon::xbmc::"));
  EXPECT_TRUE(isParameterRightType("p.XBMCAddon::xbmc::PlayList","p.PlayList","XBMCAddon::xbmc::"));
  EXPECT_TRUE(isParameterRightType("p.PlayList","p.XBMCAddon::xbmc::PlayList","XBMCAddon::xbmc::"));
}

