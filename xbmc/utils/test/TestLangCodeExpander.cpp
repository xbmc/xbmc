/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "utils/LangCodeExpander.h"

#include "gtest/gtest.h"

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
