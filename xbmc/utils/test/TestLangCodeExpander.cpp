/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"

#include <gtest/gtest.h>

TEST(TestLangCodeExpander, ConvertISO6391ToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertISO6391ToISO6392B("en", varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestLangCodeExpander, ConvertToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertToISO6392B("en", varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
