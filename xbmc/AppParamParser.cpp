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

#include "AppParamParser.h"
#include "utils/StringUtils.h"
#include "CompileInfo.h"
#include <stdlib.h>

CAppParamParser::CAppParamParser()
{
}

void CAppParamParser::Parse(const char* argv[], int nArgs, CXBMCOptions &options)
{
  if (nArgs > 1)
  {
    for (int i = 1; i < nArgs; i++)
      ParseArg(argv[i], options);
  }
}

void CAppParamParser::DisplayVersion()
{
  std::string appName = CCompileInfo::GetAppName();
  std::string version;

  if (strlen(CCompileInfo::GetSuffix()) == 0)
    version = StringUtils::Format("%d.%d Git: %s", CCompileInfo::GetMajor(), CCompileInfo::GetMinor(), CCompileInfo::GetSCMID());
  else
    version = StringUtils::Format("%d.%d-%s Git: %s", CCompileInfo::GetMajor(), CCompileInfo::GetMinor(), CCompileInfo::GetSuffix(), CCompileInfo::GetSCMID());

  printf("%s Media Center %s\n", version.c_str(), appName.c_str());
  printf("Copyright (C) 2005-2013 Team %s - http://kodi.tv\n", appName.c_str());
  exit(0);
}

void CAppParamParser::DisplayHelp()
{
  std::string appName = CCompileInfo::GetAppName();
  std::string lcAppName = appName;
  StringUtils::ToLower(lcAppName);

  printf("Usage: %s [OPTION]... [FILE]...\n\n", lcAppName.c_str());
  printf("Arguments:\n");
  printf("  -fs\t\t\tRuns %s in full screen\n", appName.c_str());
  printf("  --standalone\t\t%s runs in a stand alone environment without a window \n", appName.c_str());
  printf("\t\t\tmanager and supporting applications. For example, that\n");
  printf("\t\t\tenables network settings.\n");
  printf("  -p or --portable\t%s will look for configurations in install folder instead of ~/.%s\n", appName.c_str(), lcAppName.c_str());
  printf("  --version\t\tPrint version information\n");
  printf("  --settings=<filename>\t\tLoads specified file after advancedsettings.xml replacing any settings specified\n");
  printf("  \t\t\t\tspecified file must exist in special://xbmc/system/\n");
  exit(0);
}

void CAppParamParser::ParseArg(const std::string &arg, CXBMCOptions &options)
{
  if (arg == "-fs" || arg == "--fullscreen")
    options.fullscreen = true;
  else if (arg == "-h" || arg == "--help")
    DisplayHelp();
  else if (arg == "-v" || arg == "--version")
    DisplayVersion();
  else if (arg == "--standalone")
    options.standalone = true;
  else if (arg == "-p" || arg  == "--portable")
    options.portable = true;
  else if (arg.substr(0, 11) == "--settings=")
    options.settings.push_back(arg.substr(11));
}
