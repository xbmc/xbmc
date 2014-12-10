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

#include "utils/AliasShortcutUtils.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"

#if defined(TARGET_DARWIN_OSX)
#include "osx/DarwinUtils.h"
#endif
#include "gtest/gtest.h"

TEST(TestAliasShortcutUtils, IsAliasShortcut)
{
  XFILE::CFile *tmpFile = XBMC_CREATETEMPFILE("noaliastest");
  std::string noalias = XBMC_TEMPFILEPATH(tmpFile);

#if defined(TARGET_DARWIN_OSX)
  XFILE::CFile *aliasDestFile = XBMC_CREATETEMPFILE("aliastest");
  std::string alias = XBMC_TEMPFILEPATH(aliasDestFile);

  //we only need the path here so delete the alias file
  //which will be recreated as shortcut later:
  XBMC_DELETETEMPFILE(aliasDestFile);

  // create alias from a pointing to /Volumes
  CDarwinUtils::CreateAliasShortcut(alias, "/Volumes");
  EXPECT_TRUE(IsAliasShortcut(alias));
  XFILE::CFile::Delete(alias);

  // volumes is not a shortcut but a dir
  EXPECT_FALSE(IsAliasShortcut("/Volumes"));
#endif

  // a regular file is not a shortcut
  EXPECT_FALSE(IsAliasShortcut(noalias));
  XBMC_DELETETEMPFILE(tmpFile);

  // empty string is not an alias
  std::string emptyString;
  EXPECT_FALSE(IsAliasShortcut(emptyString));

  // non-existent file is no alias
  std::string nonExistingFile="/IDontExistsNormally/somefile.txt";
  EXPECT_FALSE(IsAliasShortcut(nonExistingFile));
}

TEST(TestAliasShortcutUtils, TranslateAliasShortcut)
{
  XFILE::CFile *tmpFile = XBMC_CREATETEMPFILE("noaliastest");
  std::string noalias = XBMC_TEMPFILEPATH(tmpFile);
  std::string noaliastemp = noalias;

#if defined(TARGET_DARWIN_OSX)
  XFILE::CFile *aliasDestFile = XBMC_CREATETEMPFILE("aliastest");
  std::string alias = XBMC_TEMPFILEPATH(aliasDestFile);

  //we only need the path here so delete the alias file
  //which will be recreated as shortcut later:
  XBMC_DELETETEMPFILE(aliasDestFile);

  // create alias from a pointing to /Volumes
  CDarwinUtils::CreateAliasShortcut(alias, "/Volumes");

  // resolve the shortcut
  TranslateAliasShortcut(alias);
  EXPECT_STREQ("/Volumes", alias.c_str());
  XFILE::CFile::Delete(alias);
#endif

  // translating a non-shortcut url should result in no change...
  TranslateAliasShortcut(noaliastemp);
  EXPECT_STREQ(noaliastemp.c_str(), noalias.c_str());
  XBMC_DELETETEMPFILE(tmpFile);

  //translate empty should stay empty
  std::string emptyString;
  TranslateAliasShortcut(emptyString);
  EXPECT_STREQ("", emptyString.c_str());

  // translate non-existent file should result in no change...
  std::string nonExistingFile="/IDontExistsNormally/somefile.txt";
  std::string resolvedNonExistingFile=nonExistingFile;
  TranslateAliasShortcut(resolvedNonExistingFile);
  EXPECT_STREQ(resolvedNonExistingFile.c_str(), nonExistingFile.c_str());
}
