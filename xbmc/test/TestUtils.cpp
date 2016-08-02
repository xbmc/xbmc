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

#include "TestUtils.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#include <cstdlib>
#include <climits>
#include <ctime>
#endif

class CTempFile : public XFILE::CFile
{
public:
  CTempFile(){};
  ~CTempFile()
  {
    Delete();
  }
  bool Create(const std::string &suffix)
  {
    char tmp[MAX_PATH];

    m_ptempFileDirectory = CSpecialProtocol::TranslatePath("special://temp/");
    m_ptempFilePath = m_ptempFileDirectory + "xbmctempfileXXXXXX";
    m_ptempFilePath += suffix;
    if (m_ptempFilePath.length() >= MAX_PATH)
    {
      m_ptempFilePath = "";
      return false;
    }
    strcpy(tmp, m_ptempFilePath.c_str());

#ifdef TARGET_WINDOWS
    if (!GetTempFileName(CSpecialProtocol::TranslatePath("special://temp/").c_str(),
                         "xbmctempfile", 0, tmp))
    {
      m_ptempFilePath = "";
      return false;
    }
    m_ptempFilePath = tmp;
#else
    int fd;
    if ((fd = mkstemps(tmp, suffix.length())) < 0)
    {
      m_ptempFilePath = "";
      return false;
    }
    close(fd);
    m_ptempFilePath = tmp;
#endif

    OpenForWrite(m_ptempFilePath.c_str(), true);
    return true;
  }
  bool Delete()
  {
    Close();
    return CFile::Delete(m_ptempFilePath);
  };
  std::string getTempFilePath() const
  {
    return m_ptempFilePath;
  }
  std::string getTempFileDirectory() const
  {
    return m_ptempFileDirectory;
  }
private:
  std::string m_ptempFilePath;
  std::string m_ptempFileDirectory;
};

CXBMCTestUtils::CXBMCTestUtils()
{
  probability = 0.01;
}

CXBMCTestUtils &CXBMCTestUtils::Instance()
{
  static CXBMCTestUtils instance;
  return instance;
}

std::string CXBMCTestUtils::ReferenceFilePath(const std::string& path)
{
  return CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder("special://xbmc", path));
}

bool CXBMCTestUtils::SetReferenceFileBasePath()
{
  std::string xbmcPath = CUtil::GetHomePath();
  if (xbmcPath.empty())
    return false;

  /* Set xbmc, xbmcbin and home path */
  CSpecialProtocol::SetXBMCPath(xbmcPath);
  CSpecialProtocol::SetXBMCBinPath(xbmcPath);
  CSpecialProtocol::SetHomePath(URIUtils::AddFileToFolder(xbmcPath, "portable_data"));

  return true;
}

XFILE::CFile *CXBMCTestUtils::CreateTempFile(std::string const& suffix)
{
  CTempFile *f = new CTempFile();
  if (f->Create(suffix))
    return f;
  delete f;
  return NULL;
}

bool CXBMCTestUtils::DeleteTempFile(XFILE::CFile *tempfile)
{
  if (!tempfile)
    return true;
  CTempFile *f = static_cast<CTempFile*>(tempfile);
  bool retval = f->Delete();
  delete f;
  return retval;
}

std::string CXBMCTestUtils::TempFilePath(XFILE::CFile const* const tempfile)
{
  if (!tempfile)
    return "";
  CTempFile const* const f = static_cast<CTempFile const* const>(tempfile);
  return f->getTempFilePath();
}

std::string CXBMCTestUtils::TempFileDirectory(XFILE::CFile const* const tempfile)
{
  if (!tempfile)
    return "";
  CTempFile const* const f = static_cast<CTempFile const* const>(tempfile);
  return f->getTempFileDirectory();
}

XFILE::CFile *CXBMCTestUtils::CreateCorruptedFile(std::string const& strFileName,
  std::string const& suffix)
{
  XFILE::CFile inputfile, *tmpfile = CreateTempFile(suffix);
  unsigned char buf[20], tmpchar;
  unsigned int size, i;

  if (tmpfile && inputfile.Open(strFileName))
  {
    srand(time(NULL));
    while ((size = inputfile.Read(buf, sizeof(buf))) > 0)
    {
      for (i = 0; i < size; i++)
      {
        if ((rand() % RAND_MAX) < (probability * RAND_MAX))
        {
          tmpchar = buf[i];
          do
          {
            buf[i] = (rand() % 256);
          } while (buf[i] == tmpchar);
        }
      }
      if (tmpfile->Write(buf, size) < 0)
      {
        inputfile.Close();
        tmpfile->Close();
        DeleteTempFile(tmpfile);
        return NULL;
      }
    }
    inputfile.Close();
    tmpfile->Close();
    return tmpfile;
  }
  delete tmpfile;
  return NULL;
}


std::vector<std::string> &CXBMCTestUtils::getTestFileFactoryReadUrls()
{
  return TestFileFactoryReadUrls;
}

std::vector<std::string> &CXBMCTestUtils::getTestFileFactoryWriteUrls()
{
  return TestFileFactoryWriteUrls;
}

std::string &CXBMCTestUtils::getTestFileFactoryWriteInputFile()
{
  return TestFileFactoryWriteInputFile;
}

void CXBMCTestUtils::setTestFileFactoryWriteInputFile(std::string const& file)
{
  TestFileFactoryWriteInputFile = file;
}

std::vector<std::string> &CXBMCTestUtils::getAdvancedSettingsFiles()
{
  return AdvancedSettingsFiles;
}

std::vector<std::string> &CXBMCTestUtils::getGUISettingsFiles()
{
  return GUISettingsFiles;
}

static const char usage[] =
"XBMC Test Suite\n"
"Usage: xbmc-test [options]\n"
"\n"
"The following options are recognized by the xbmc-test program.\n"
"\n"
"  --add-testfilefactory-readurl [URL]\n"
"    Add a url to be used int the TestFileFactory read tests.\n"
"\n"
"  --add-testfilefactory-readurls [URLS]\n"
"    Add multiple urls from a ',' delimited string of urls to be used\n"
"    in the TestFileFactory read tests.\n"
"\n"
"  --add-testfilefactory-writeurl [URL]\n"
"    Add a url to be used int the TestFileFactory write tests.\n"
"\n"
"  --add-testfilefactory-writeurls [URLS]\n"
"    Add multiple urls from a ',' delimited string of urls to be used\n"
"    in the TestFileFactory write tests.\n"
"\n"
"  --set-testfilefactory-writeinputfile [FILE]\n"
"    Set the path to the input file used in the TestFileFactory write tests.\n"
"\n"
"  --add-advancedsettings-file [FILE]\n"
"    Add an advanced settings file to be loaded in test cases that use them.\n"
"\n"
"  --add-advancedsettings-files [FILES]\n"
"    Add multiple advanced settings files from a ',' delimited string of\n"
"    files to be loaded in test cases that use them.\n"
"\n"
"  --add-guisettings-file [FILE]\n"
"    Add a GUI settings file to be loaded in test cases that use them.\n"
"\n"
"  --add-guisettings-files [FILES]\n"
"    Add multiple GUI settings files from a ',' delimited string of\n"
"    files to be loaded in test cases that use them.\n"
"\n"
"  --set-probability [PROBABILITY]\n"
"    Set the probability variable used by the file corrupting functions.\n"
"    The variable should be a double type from 0.0 to 1.0. Values given\n"
"    less than 0.0 are treated as 0.0. Values greater than 1.0 are treated\n"
"    as 1.0. The default probability is 0.01.\n"
;

void CXBMCTestUtils::ParseArgs(int argc, char **argv)
{
  int i;
  std::string arg;
  for (i = 1; i < argc; i++)
  {
    arg = argv[i];
    if (arg == "--add-testfilefactory-readurl")
    {
      TestFileFactoryReadUrls.push_back(argv[++i]);
    }
    else if (arg == "--add-testfilefactory-readurls")
    {
      arg = argv[++i];
      std::vector<std::string> urls = StringUtils::Split(arg, ",");
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); ++it)
        TestFileFactoryReadUrls.push_back(*it);
    }
    else if (arg == "--add-testfilefactory-writeurl")
    {
      TestFileFactoryWriteUrls.push_back(argv[++i]);
    }
    else if (arg == "--add-testfilefactory-writeurls")
    {
      arg = argv[++i];
      std::vector<std::string> urls = StringUtils::Split(arg, ",");
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); ++it)
        TestFileFactoryWriteUrls.push_back(*it);
    }
    else if (arg == "--set-testfilefactory-writeinputfile")
    {
      TestFileFactoryWriteInputFile = argv[++i];
    }
    else if (arg == "--add-advancedsettings-file")
    {
      AdvancedSettingsFiles.push_back(argv[++i]);
    }
    else if (arg == "--add-advancedsettings-files")
    {
      arg = argv[++i];
      std::vector<std::string> urls = StringUtils::Split(arg, ",");
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); ++it)
        AdvancedSettingsFiles.push_back(*it);
    }
    else if (arg == "--add-guisettings-file")
    {
      GUISettingsFiles.push_back(argv[++i]);
    }
    else if (arg == "--add-guisettings-files")
    {
      arg = argv[++i];
      std::vector<std::string> urls = StringUtils::Split(arg, ",");
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); ++it)
        GUISettingsFiles.push_back(*it);
    }
    else if (arg == "--set-probability")
    {
      probability = atof(argv[++i]);
      if (probability < 0.0)
        probability = 0.0;
      else if (probability > 1.0)
        probability = 1.0;
    }
    else
    {
      std::cerr << usage;
      exit(EXIT_FAILURE);
    }
  }
}

std::string CXBMCTestUtils::getNewLineCharacters() const
{
#ifdef TARGET_WINDOWS
  return "\r\n";
#else
  return "\n";
#endif
}
