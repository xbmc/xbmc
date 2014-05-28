/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef TEST_UTILS_LANGCODEEXPANDER_H_INCLUDED
#define TEST_UTILS_LANGCODEEXPANDER_H_INCLUDED
#include "utils/LangCodeExpander.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(TestLangCodeExpander, ConvertTwoToThreeCharCode)
{
  CStdString refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertTwoToThreeCharCode(varstr, "en");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestLangCodeExpander, ConvertToThreeCharCode)
{
  CStdString refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertToThreeCharCode(varstr, "en");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
