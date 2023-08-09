/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include <tinyxml2.h>

class CXBMCTinyXML2 : public tinyxml2::XMLDocument
{
public:
  CXBMCTinyXML2() = default;
  bool LoadFile(const std::string& filename);
  bool LoadFile(FILE* file);
  bool SaveFile(const std::string& filename) const;
  bool Parse(std::string_view inputdata);
  bool Parse(std::string&& inputdata);

private:
  bool ParseHelper(size_t pos, std::string&& inputdata);
};
