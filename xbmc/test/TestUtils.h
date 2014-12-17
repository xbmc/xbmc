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
#pragma once

#include <string>
#include <vector>

namespace XFILE
{
  class CFile;
}

class CXBMCTestUtils
{
public:
  static CXBMCTestUtils &Instance();

  /* ReferenceFilePath() is used to prepend a path with the location to the
   * xbmc-test binary. It's assumed the test suite program will only be run
   * with xbmc-test residing in the source tree.
   */
  std::string ReferenceFilePath(const std::string& path);

  /* Function to set the reference file base path. */
  bool SetReferenceFileBasePath();

  /* Function used in creating a temporary file. It accepts a parameter
   * 'suffix' to append to the end of the tempfile path. The temporary
   * file is return as a XFILE::CFile object.
   */
  XFILE::CFile *CreateTempFile(std::string const& suffix);

  /* Function used to close and delete a temporary file previously created
   * using CreateTempFile().
   */
  bool DeleteTempFile(XFILE::CFile *tempfile);

  /* Function to get path of a tempfile */
  std::string TempFilePath(XFILE::CFile const* const tempfile);

  /* Get the containing directory of a tempfile */
  std::string TempFileDirectory(XFILE::CFile const* const tempfile);

  /* Functions to get variables used in the TestFileFactory tests. */
  std::vector<std::string> &getTestFileFactoryReadUrls();

  /* Function to get variables used in the TestFileFactory tests. */
  std::vector<std::string> &getTestFileFactoryWriteUrls();

  /* Function to get the input file used in the TestFileFactory.Write tests. */
  std::string &getTestFileFactoryWriteInputFile();

  /* Function to set the input file used in the TestFileFactory.Write tests */
  void setTestFileFactoryWriteInputFile(std::string const& file);

  /* Function to get advanced settings files. */
  std::vector<std::string> &getAdvancedSettingsFiles();

  /* Function to get GUI settings files. */
  std::vector<std::string> &getGUISettingsFiles();

  /* Function used in creating a corrupted file. The parameters are a URL
   * to the original file to be corrupted and a suffix to append to the
   * path of the newly created file. This will return a XFILE::CFile
   * object which is itself a tempfile object which can be used with the
   * tempfile functions of this utility class.
   */
  XFILE::CFile *CreateCorruptedFile(std::string const& strFileName,
                                    std::string const& suffix);

  /* Function to parse command line options */
  void ParseArgs(int argc, char **argv);

  /* Function to return the newline characters for this platform */
  std::string getNewLineCharacters() const;
private:
  CXBMCTestUtils();
  CXBMCTestUtils(CXBMCTestUtils const&);
  void operator=(CXBMCTestUtils const&);

  std::vector<std::string> TestFileFactoryReadUrls;
  std::vector<std::string> TestFileFactoryWriteUrls;
  std::string TestFileFactoryWriteInputFile;

  std::vector<std::string> AdvancedSettingsFiles;
  std::vector<std::string> GUISettingsFiles;

  double probability;
};

#define XBMC_REF_FILE_PATH(s) CXBMCTestUtils::Instance().ReferenceFilePath(s)
#define XBMC_CREATETEMPFILE(a) CXBMCTestUtils::Instance().CreateTempFile(a)
#define XBMC_DELETETEMPFILE(a) CXBMCTestUtils::Instance().DeleteTempFile(a)
#define XBMC_TEMPFILEPATH(a) CXBMCTestUtils::Instance().TempFilePath(a)
#define XBMC_CREATECORRUPTEDFILE(a, b) \
  CXBMCTestUtils::Instance().CreateCorruptedFile(a, b)
