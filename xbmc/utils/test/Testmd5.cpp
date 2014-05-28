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

#ifndef TEST_UTILS_MD5_H_INCLUDED
#define TEST_UTILS_MD5_H_INCLUDED
#include "utils/md5.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(Testmd5, ZeroLengthString)
{
  XBMC::XBMC_MD5 a;
  CStdString refdigest, vardigest;

  refdigest = "D41D8CD98F00B204E9800998ECF8427E";
  a.append("");
  a.getDigest(vardigest);
  EXPECT_STREQ(refdigest.c_str(), vardigest.c_str());
}

TEST(Testmd5, String1)
{
  XBMC::XBMC_MD5 a;
  CStdString refdigest, vardigest;

  refdigest = "9E107D9D372BB6826BD81D3542A419D6";
  a.append("The quick brown fox jumps over the lazy dog");
  a.getDigest(vardigest);
  EXPECT_STREQ(refdigest.c_str(), vardigest.c_str());
}

TEST(Testmd5, String2)
{
  XBMC::XBMC_MD5 a;
  CStdString refdigest, vardigest;

  refdigest = "E4D909C290D0FB1CA068FFADDF22CBD0";
  a.append("The quick brown fox jumps over the lazy dog.");
  a.getDigest(vardigest);
  EXPECT_STREQ(refdigest.c_str(), vardigest.c_str());
}
