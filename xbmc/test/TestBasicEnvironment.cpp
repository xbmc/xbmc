/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "TestBasicEnvironment.h"
#include "TestUtils.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "powermanagement/PowerManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "Util.h"

#include <cstdio>
#include <cstdlib>
#include <climits>

void TestBasicEnvironment::SetUp()
{
  char *tmp;
  CStdString xbmcTempPath;
  XFILE::CFile *f;

  /* NOTE: The below is done to fix memleak warning about unitialized variable
   * in xbmcutil::GlobalsSingleton<CAdvancedSettings>::getInstance().
   */
  g_advancedSettings.Initialize();

  if (!CXBMCTestUtils::Instance().SetReferenceFileBasePath())
    SetUpError();
  CXBMCTestUtils::Instance().setTestFileFactoryWriteInputFile(
    XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt")
  );

//for darwin set framework path - else we get assert
//in guisettings init below
#ifdef TARGET_DARWIN
  CStdString frameworksPath = CUtil::GetFrameworksPath();
  CSpecialProtocol::SetXBMCFrameworksPath(frameworksPath);    
#endif
  /* TODO: Something should be done about all the asserts in GUISettings so
   * that the initialization of these components won't be needed.
   */
  g_powerManager.Initialize();
  g_guiSettings.Initialize();

  /* Create a temporary directory and set it to be used throughout the
   * test suite run.
   */
#ifndef _LINUX
  TCHAR lpTempPathBuffer[MAX_PATH];
  if (!GetTempPath(MAX_PATH, lpTempPathBuffer))
    SetUpError();
  xbmcTempPath = lpTempPathBuffer;
  if (!GetTempFileName(xbmcTempPath.c_str(), "xbmctempdir", 0, lpTempPathBuffer))
    SetUpError();
  DeleteFile(lpTempPathBuffer);
  if (!CreateDirectory(lpTempPathBuffer, NULL))
    SetUpError();
  CSpecialProtocol::SetTempPath(lpTempPathBuffer);
#else
  char buf[MAX_PATH];
  (void)xbmcTempPath;
  strcpy(buf, "/tmp/xbmctempdirXXXXXX");
  if ((tmp = mkdtemp(buf)) == NULL)
    SetUpError();
  CSpecialProtocol::SetTempPath(tmp);
#endif

  /* Create and delete a tempfile to initialize the VFS (really to initialize
   * CLibcdio). This is done so that the initialization of the VFS does not
   * affect the performance results of the test cases.
   */
  /* TODO: Make the initialization of the VFS here optional so it can be
   * testable in a test case.
   */
  f = XBMC_CREATETEMPFILE("");
  if (!f || !XBMC_DELETETEMPFILE(f))
  {
    TearDown();
    SetUpError();
  }
}

void TestBasicEnvironment::TearDown()
{
  CStdString xbmcTempPath = CSpecialProtocol::TranslatePath("special://temp/");
  XFILE::CDirectory::Remove(xbmcTempPath);
}

void TestBasicEnvironment::SetUpError()
{
  fprintf(stderr, "Setup of basic environment failed.\n");
  exit(EXIT_FAILURE);
}
