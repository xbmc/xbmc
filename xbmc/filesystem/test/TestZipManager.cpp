/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/ZipManager.h"
#include "utils/RegExp.h"

#include <gtest/gtest.h>

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
