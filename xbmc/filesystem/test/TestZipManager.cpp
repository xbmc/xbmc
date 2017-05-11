/*
 *      Copyright (C) 2017 Team XBMC
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

#include "filesystem/ZipManager.h"

#include "gtest/gtest.h"

TEST(TestZipManager, PathTraversal)
{
  ASSERT_TRUE(std::regex_search("..", PATH_TRAVERSAL));
  ASSERT_TRUE(std::regex_search("../test.txt", PATH_TRAVERSAL));
  ASSERT_TRUE(std::regex_search("..\\test.txt", PATH_TRAVERSAL));
  ASSERT_TRUE(std::regex_search("test/../test.txt", PATH_TRAVERSAL));
  ASSERT_TRUE(std::regex_search("test\\../test.txt", PATH_TRAVERSAL));
  ASSERT_TRUE(std::regex_search("test\\..\\test.txt", PATH_TRAVERSAL));

  ASSERT_FALSE(std::regex_search("...", PATH_TRAVERSAL));
  ASSERT_FALSE(std::regex_search("..test.txt", PATH_TRAVERSAL));
  ASSERT_FALSE(std::regex_search("test.txt..", PATH_TRAVERSAL));
  ASSERT_FALSE(std::regex_search("test..test.txt", PATH_TRAVERSAL));
}
