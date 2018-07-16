/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/MessagePrinter.h"
#include "CompileInfo.h"

#include <stdio.h>

void CMessagePrinter::DisplayMessage(const std::string& message)
{
  fprintf(stdout, "%s\n", message.c_str());
}

void CMessagePrinter::DisplayWarning(const std::string& warning)
{
  fprintf(stderr, "%s\n", warning.c_str());
}

void CMessagePrinter::DisplayError(const std::string& error)
{
  fprintf(stderr,"%s\n", error.c_str());
}

void CMessagePrinter::DisplayHelpMessage(const std::vector<std::pair<std::string, std::string>>& help)
{
  //very crude implementation, pretty it up when possible
  std::string message;
  for (const auto& line : help)
  {
    message.append(line.first + "\t" + line.second + "\n");
  }

  fprintf(stdout, "%s\n", message.c_str());
}
