/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringFormat.h"

#include "utils/LangCodeExpander.h"

std::string StringFormat::AsBcp47Tag(const std::string& input, void* data)
{
  std::string output;
  g_LangCodeExpander.ConvertToBcp47(input, output);

  return output;
}
