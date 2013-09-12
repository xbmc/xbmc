/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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

#include "FileItem.h"
#include "games/GameClient.h"
#include "games/GameFileLoader.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "gtest/gtest.h"

using namespace ADDON;
using namespace GAME_INFO;
using namespace GAMES;

TEST(TestGameFileLoader, CGameFileLoaderUseHD)
{
  CGameFileLoaderUseHD loaderHD;

  GameClientConfig gc;
  GameClientConfig gc_nes;   gc_nes.extensions.insert(".nes");
  GameClientConfig gc_smc;   gc_smc.extensions.insert(".smc");
  GameClientConfig gc_noVFS; gc_noVFS.bAllowVFS = false;

  // Scenario 1: Local file
  CFileItem localFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.nes"), false);
  EXPECT_TRUE(loaderHD.CanLoad(gc, localFile));
  EXPECT_TRUE(loaderHD.CanLoad(gc_nes, localFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_smc, localFile));
  EXPECT_TRUE(loaderHD.CanLoad(gc_noVFS, localFile));

  // Scenario 2: smb:// file
  CFileItem smbFile("smb://server/share/SmbFile.nes", false);
  EXPECT_FALSE(loaderHD.CanLoad(gc, smbFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_nes, smbFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_smc, smbFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_noVFS, smbFile));

  // Scenario 3: zip:// file
  CStdString strZipPath;
  URIUtils::CreateArchivePath(strZipPath, "zip", XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), "ZipFile.nes");
  CFileItem zipFile(strZipPath, false);
  EXPECT_FALSE(loaderHD.CanLoad(gc, zipFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_nes, zipFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_smc, zipFile));
  EXPECT_FALSE(loaderHD.CanLoad(gc_noVFS, zipFile));
}

TEST(TestGameFileLoader, CGameFileLoaderUseVFS)
{
  CGameFileLoaderUseVFS loaderVFS;

  GameClientConfig gc;
  GameClientConfig gc_nes;   gc_nes.extensions.insert(".nes");
  GameClientConfig gc_smc;   gc_smc.extensions.insert(".smc");
  GameClientConfig gc_noVFS; gc_noVFS.bAllowVFS = false;

  // Scenario 1: Local file
  CFileItem localFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.nes"), false);
  EXPECT_TRUE(loaderVFS.CanLoad(gc, localFile));
  EXPECT_TRUE(loaderVFS.CanLoad(gc_nes, localFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_smc, localFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_noVFS, localFile));

  // Scenario 2: smb:// file
  CFileItem smbFile("smb://server/share/SmbFile.nes", false);
  EXPECT_TRUE(loaderVFS.CanLoad(gc, smbFile));
  EXPECT_TRUE(loaderVFS.CanLoad(gc_nes, smbFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_smc, smbFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_noVFS, smbFile));

  // Scenario 3: zip:// file
  CStdString strZipPath;
  URIUtils::CreateArchivePath(strZipPath, "zip", XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), "ZipFile.nes");
  CFileItem zipFile(strZipPath, false);
  EXPECT_TRUE(loaderVFS.CanLoad(gc, zipFile));
  EXPECT_TRUE(loaderVFS.CanLoad(gc_nes, zipFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_smc, zipFile));
  EXPECT_FALSE(loaderVFS.CanLoad(gc_noVFS, zipFile));
}

TEST(TestGameFileLoader, CGameFileLoaderUseParentZip)
{
  CGameFileLoaderUseParentZip loaderParentZip;
  
  GameClientConfig gc;
  GameClientConfig gc_zip;     gc_zip.extensions.insert(".zip");
  GameClientConfig gc_smc;     gc_smc.extensions.insert(".smc");
  GameClientConfig gc_zip_smc; gc_zip_smc.extensions.insert(".zip"); gc_zip_smc.extensions.insert(".smc");
  GameClientConfig gc_noVFS;   gc_noVFS.bAllowVFS = false;

  // Scenario 1: Parent zip (zip:// file)
  CStdString strZipPath;
  URIUtils::CreateArchivePath(strZipPath, "zip", XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), "ZipFile.smc");
  CFileItem zipFile(strZipPath, false);
  EXPECT_TRUE(loaderParentZip.CanLoad(gc, zipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip, zipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_smc, zipFile));
  EXPECT_TRUE(loaderParentZip.CanLoad(gc_zip_smc, zipFile));
  EXPECT_TRUE(loaderParentZip.CanLoad(gc_noVFS, zipFile)); // This test ignores VFS constraint

  // Scenario 2: Not inside a zip
  CFileItem localFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), false);
  EXPECT_FALSE(loaderParentZip.CanLoad(gc, localFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip, localFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_smc, localFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip_smc, localFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_noVFS, localFile));
  
  // Scenario 3: Not in a zip's root directory
  CStdString strDeepZipPath;
  URIUtils::CreateArchivePath(strDeepZipPath, "zip", XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), "folder/ZipFile.smc");
  CFileItem deepZipFile(strDeepZipPath, false);
  EXPECT_FALSE(loaderParentZip.CanLoad(gc, deepZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip, deepZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_smc, deepZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip_smc, deepZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_noVFS, deepZipFile));

  // Scenario 4: Inside a zip file, embedded in another zip file
  CStdString strOuterZipPath;
  URIUtils::CreateArchivePath(strOuterZipPath, "zip", XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), "InnerZipFile.zip");
  CStdString strInnerZipPath;
  URIUtils::CreateArchivePath(strInnerZipPath, "zip", strOuterZipPath, "ZipFile.smc");
  CFileItem embeddedZipFile(strInnerZipPath, false);
  EXPECT_FALSE(loaderParentZip.CanLoad(gc, embeddedZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip, embeddedZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_smc, embeddedZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip_smc, embeddedZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_noVFS, embeddedZipFile));

  // Scenario 5: Parent zip is not local
  CStdString strSmbZipPath;
  URIUtils::CreateArchivePath(strSmbZipPath, "zip", "smb://server/share/SmbFile.zip", "ZipFile.smc");
  CFileItem smbZipFile(strSmbZipPath, false);
  EXPECT_FALSE(loaderParentZip.CanLoad(gc, smbZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip, smbZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_smc, smbZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_zip_smc, smbZipFile));
  EXPECT_FALSE(loaderParentZip.CanLoad(gc_noVFS, smbZipFile));
}

TEST(TestGameFileLoader, CGameFileLoaderEnterZip)
{
  CGameFileLoaderEnterZip loaderEnterZip;
  
  GameClientConfig gc;
  GameClientConfig gc_smc;     gc_smc.extensions.insert(".smc");
  GameClientConfig gc_bin;     gc_bin.extensions.insert(".bin");
  GameClientConfig gc_nes_smc; gc_nes_smc.extensions.insert(".nes"); gc_nes_smc.extensions.insert(".smc");
  GameClientConfig gc_noVFS;   gc_smc.extensions.insert(".smc"); gc_noVFS.bAllowVFS = false;
  
  // Scenario 1: Not a zip
  CFileItem localFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.nes"), false);
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc, localFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_smc, localFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_bin, localFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_nes_smc, localFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_noVFS, localFile));
  
  // Scenario 2: Enter the zip file "xbmc/games/test/LocalFile.zip"
  // Contents of zip are:
  //   * giggley giggley garbear.smc
  //   * hungry hungry hippos.smc
  //   * pretty pretty princess.nes
  CFileItem localZipFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), false);
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc, localZipFile)); // Must specify extensions (currently fails test)
  EXPECT_TRUE(loaderEnterZip.CanLoad(gc_smc, localZipFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_bin, localZipFile));
  EXPECT_TRUE(loaderEnterZip.CanLoad(gc_nes_smc, localZipFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_noVFS, localZipFile));
  EXPECT_FALSE(loaderEnterZip.CanLoad(gc_noVFS, localZipFile));
}

TEST(TestGameFileLoader, CanOpen)
{
  // Test CGameFileLoader::CanOpen() with useStrategies = false, because
  // strategies have already already been tested above

  GameClientConfig gc("gameclient.test");
  GameClientConfig gc_nes; gc_nes.extensions.insert(".nes");
  GameClientConfig gc_bin; gc_bin.extensions.insert(".bin");
  CFileItem file;

  EXPECT_TRUE(CGameFileLoader::CanOpen(file, gc));

  file.SetProperty("gameclient", "gameclient.badegg");
  EXPECT_FALSE(CGameFileLoader::CanOpen(file, gc));

  file.ClearProperty("gameclient");
  gc.platforms.push_back(PLATFORM_NINTENDO_64);
  EXPECT_TRUE(CGameFileLoader::CanOpen(file, gc));
  file.GetGameInfoTag()->SetPlatform("Playstation");
  EXPECT_FALSE(CGameFileLoader::CanOpen(file, gc));
  gc.platforms.clear();
  EXPECT_TRUE(CGameFileLoader::CanOpen(file, gc));

  file.SetPath(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.nes"));
  EXPECT_TRUE(CGameFileLoader::CanOpen(file, gc));
  EXPECT_TRUE(CGameFileLoader::CanOpen(file, gc_nes));
  EXPECT_FALSE(CGameFileLoader::CanOpen(file, gc_bin));

  // Test entering .zip files
  CFileItem localZipFile(XBMC_REF_FILE_PATH("xbmc/games/test/LocalFile.zip"), false);
  EXPECT_TRUE(CGameFileLoader::CanOpen(localZipFile, gc)); // No extensions specified, will try to load zip directly
  EXPECT_TRUE(CGameFileLoader::CanOpen(localZipFile, gc_nes));
  EXPECT_FALSE(CGameFileLoader::CanOpen(localZipFile, gc_bin));
}
