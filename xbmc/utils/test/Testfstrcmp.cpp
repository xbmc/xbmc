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

#include "utils/fstrcmp.h"
#include "utils/StringUtils.h"

#include "gtest/gtest.h"

TEST(Testfstrcmp, General)
{
  std::string refstr, varstr, refresult, varresult;
  refstr = "Testfstrcmp test string";
  varstr = refstr;

  /* NOTE: Third parameter is not used at all in fstrcmp. */
  refresult = "1.000000";
  varresult = StringUtils::Format("%.6f", fstrcmp(refstr.c_str(), varstr.c_str(), 0.0));
  EXPECT_STREQ(refresult.c_str(), varresult.c_str());

  varstr = "Testfstrcmp_test_string";
  refresult = "0.913043";
  varresult = StringUtils::Format("%.6f", fstrcmp(refstr.c_str(), varstr.c_str(), 0.0));
  EXPECT_STREQ(refresult.c_str(), varresult.c_str());

  varstr = "";
  refresult = "0.000000";
  varresult = StringUtils::Format("%.6f", fstrcmp(refstr.c_str(), varstr.c_str(), 0.0));
  EXPECT_STREQ(refresult.c_str(), varresult.c_str());
}
