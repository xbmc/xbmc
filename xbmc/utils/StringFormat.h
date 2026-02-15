/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <string>

class StringFormat
{
public:
  using Formatter = std::function<std::string(const std::string& input, void* data)>;

  static std::string AsBcp47Tag(const std::string& input, void* data);

private:
  StringFormat() = delete;
};
