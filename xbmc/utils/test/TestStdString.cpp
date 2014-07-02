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

#include "utils/StdString.h"

#include "gtest/gtest.h"

TEST(TestStdString, CStdString)
{
  CStdString ref, var;

  ref = "CStdString test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStdString, CStdStringA)
{
  CStdStringA ref, var;

  ref = "CStdStringA test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStdString, CStdStringW)
{
  CStdStringW ref, var;

  ref = L"CStdStringW test";
  var = ref;
  EXPECT_STREQ(ref.c_str(), var.c_str());
}
