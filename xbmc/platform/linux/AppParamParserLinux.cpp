/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParserLinux.h"

#include "CompileInfo.h"
#include "application/AppParams.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

namespace
{
std::vector<std::string> availableWindowSystems = CCompileInfo::GetAvailableWindowSystems();
std::array<std::string, 1> availableLogTargets = {"console"};
std::vector<std::string> availableAudioBackends = CCompileInfo::GetAvailableAudioBackends();
std::vector<std::string> availableGlInterfaces = CCompileInfo::GetAvailableGlInterfaces();

constexpr const char* windowingText =
    R"""(
Selected window system not available: {}
    Available window systems: {}
)""";

constexpr const char* loggingText =
    R"""(
Selected logging target not available: {}
    Available log targest: {}
)""";

constexpr const char* audioBackendsText =
    R"""(
Selected audio backend not available: {}
    Available audio backends: {}
)""";

constexpr const char* glInterfaceText =
    R"""(
Selected GL interface not available: {}
    Available GL interfaces: {}
)""";

constexpr const char* helpText =
    R"""(
Linux Specific Arguments:
  --windowing=<system>  Select which windowing method to use.
                          Available window systems are: {}
  --logging=<target>    Select which log target to use (log file will always be used in conjunction).
                          Available log targets are: {}
  --audio-backend=<backend> Select which audio backend to use.
                          Available audio backends are: {}
  --gl-interface=<interface> Select which GL interface to use (X11 only).
                          Available GL interfaces are: {}
)""";

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
      GetAppParams()->SetWindowing(arg.substr(12));
    else
    {
      std::cout << StringUtils::Format(windowingText, arg.substr(12),
                                       StringUtils::Join(availableWindowSystems, ", "));
      exit(0);
    }
  }
  else if (arg.substr(0, 10) == "--logging=")
  {
    if (std::find(availableLogTargets.begin(), availableLogTargets.end(), arg.substr(10)) !=
        availableLogTargets.end())
    {
      GetAppParams()->SetLogTarget(arg.substr(10));
    }
    else
    {
      std::cout << StringUtils::Format(loggingText, arg.substr(10),
                                       StringUtils::Join(availableLogTargets, ", "));
      exit(0);
    }
  }
  else if (arg.find("--audio-backend=") != std::string::npos)
  {
    const auto argValue = arg.substr(16);
    const auto it =
        std::find(availableAudioBackends.cbegin(), availableAudioBackends.cend(), argValue);
    if (it != availableAudioBackends.cend())
    {
      GetAppParams()->SetAudioBackend(argValue);
    }
    else
    {
      std::cout << StringUtils::Format(audioBackendsText, argValue,
                                       StringUtils::Join(availableAudioBackends, ", "));

      exit(0);
    }
  }
  else if (arg.find("--gl-interface=") != std::string::npos)
  {
    const auto argValue = arg.substr(15);
    const auto it =
        std::find(availableGlInterfaces.cbegin(), availableGlInterfaces.cend(), argValue);
    if (it != availableGlInterfaces.cend())
    {
      GetAppParams()->SetGlInterface(argValue);
    }
    else
    {
      std::cout << StringUtils::Format(glInterfaceText, argValue,
                                       StringUtils::Join(availableGlInterfaces, ", "));

      exit(0);
    }
  }
}

void CAppParamParserLinux::DisplayHelp()
{
  CAppParamParser::DisplayHelp();

  std::cout << StringUtils::Format(helpText, StringUtils::Join(availableWindowSystems, ", "),
                                   StringUtils::Join(availableLogTargets, ", "),
                                   StringUtils::Join(availableAudioBackends, ", "),
                                   StringUtils::Join(availableGlInterfaces, ", "));
}
