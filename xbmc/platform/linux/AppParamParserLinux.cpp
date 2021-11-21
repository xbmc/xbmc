/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParserLinux.h"

#include "CompileInfo.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

namespace
{
std::vector<std::string> availableWindowSystems = CCompileInfo::GetAvailableWindowSystems();
std::array<std::string, 1> availableLogTargets = {"console"};

} // namespace

CAppParamParserLinux::CAppParamParserLinux() : CAppParamParser()
{
}

CAppParamParserLinux::~CAppParamParserLinux() = default;

void CAppParamParserLinux::ParseArg(const std::string& arg)
{
  CAppParamParser::ParseArg(arg);

  if (arg.substr(0, 12) == "--windowing=")
  {
    if (std::find(availableWindowSystems.begin(), availableWindowSystems.end(), arg.substr(12)) !=
        availableWindowSystems.end())
      m_windowing = arg.substr(12);
    else
    {
      std::cout << "Selected window system not available: " << arg << std::endl;
      std::cout << "    Available window systems:";
      for (const auto& windowSystem : availableWindowSystems)
        std::cout << " " << windowSystem;
      std::cout << std::endl;
      exit(0);
    }
  }
  else if (arg.substr(0, 10) == "--logging=")
  {
    if (std::find(availableLogTargets.begin(), availableLogTargets.end(), arg.substr(10)) !=
        availableLogTargets.end())
    {
      m_logTarget = arg.substr(10);
    }
    else
    {
      std::cout << "Selected logging target not available: " << arg << std::endl;
      std::cout << "    Available log targets:";
      for (const auto& logTarget : availableLogTargets)
        std::cout << " " << logTarget;
      std::cout << std::endl;
      exit(0);
    }
  }
}

void CAppParamParserLinux::DisplayHelp()
{
  CAppParamParser::DisplayHelp();

  printf("\n");
  printf("Linux Specific Arguments:\n");

  printf("  --windowing=<system>\tSelect which windowing method to use.\n");
  printf("  \t\t\t\tAvailable window systems are:");
  for (const auto& windowSystem : availableWindowSystems)
    printf(" %s", windowSystem.c_str());
  printf("\n");
  printf("  --logging=<target>\tSelect which log target to use (log file will always be used in "
         "conjunction).\n");
  printf("  \t\t\t\tAvailable log targets are:");
  for (const auto& logTarget : availableLogTargets)
    printf(" %s", logTarget.c_str());
  printf("\n");
}
