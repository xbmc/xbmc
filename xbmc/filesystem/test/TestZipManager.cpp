/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#include "filesystem/ZipManager.h"
#include "utils/RegExp.h"

#include "gtest/gtest.h"

TEST(TestZipManager, PathTraversal)
{
  CRegExp pathTraversal;
  pathTraversal.RegComp(PATH_TRAVERSAL);

  ASSERT_TRUE(pathTraversal.RegFind("..") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("..\\test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test/../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test\\../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test\\..\\test.txt") >= 0);

  ASSERT_FALSE(pathTraversal.RegFind("...") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("..test.txt") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("test.txt..") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("test..test.txt") >= 0);
}
