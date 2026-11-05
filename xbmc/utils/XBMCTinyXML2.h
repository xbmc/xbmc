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
  /*!
   * \brief Load a file, detecting its charset and converting it to UTF-8
   */
  bool LoadFile(const std::string& filename);
  bool LoadFile(FILE* file);
  bool SaveFile(const std::string& filename) const;
  /*!
   * \brief Parse UTF-8 data
   */
  bool Parse(std::string_view inputdata);
  bool Parse(std::string&& inputdata);
  /*!
   * \brief Parse data in the given charset, or detect it if empty, converting it to UTF-8
   */
  bool Parse(const std::string& inputdata, const std::string& dataCharset);
  const std::string& GetUsedCharset() const { return m_usedCharset; }

private:
  bool TryParse(const std::string& inputdata, const std::string& tryDataCharset);
  bool InternalParse(std::string_view inputdata);
  bool InternalParse(std::string&& inputdata);
  bool ParseHelper(size_t pos, std::string&& inputdata);

  std::string m_filename;
  std::string m_usedCharset;
};
