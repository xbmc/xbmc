/*
*      Copyright (C) 2005-2015 Team XBMC
*      http://kodi.tv
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