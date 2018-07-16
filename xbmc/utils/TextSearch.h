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

typedef enum TextSearchDefault
{
  SEARCH_DEFAULT_AND = 0,
  SEARCH_DEFAULT_OR,
  SEARCH_DEFAULT_NOT
} TextSearchDefault;

class CTextSearch final
{
public:
  CTextSearch(const std::string &strSearchTerms, bool bCaseSensitive = false, TextSearchDefault defaultSearchMode = SEARCH_DEFAULT_OR);

  bool Search(const std::string &strHaystack) const;
  bool IsValid(void) const;

private:
  static void GetAndCutNextTerm(std::string &strSearchTerm, std::string &strNextTerm);
  void ExtractSearchTerms(const std::string &strSearchTerm, TextSearchDefault defaultSearchMode);

  bool                     m_bCaseSensitive;
  std::vector<std::string>  m_AND;
  std::vector<std::string>  m_OR;
  std::vector<std::string>  m_NOT;
};
