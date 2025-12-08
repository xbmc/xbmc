/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

// special options that can be passed to a codec
class CDVDCodecOption
{
public:
  CDVDCodecOption(const std::string& name, const std::string& value) : m_name(name), m_value(value) {}
  std::string m_name;
  std::string m_value;
};

class CDVDCodecOptions
{
public:
  std::vector<CDVDCodecOption> m_keys;
};
