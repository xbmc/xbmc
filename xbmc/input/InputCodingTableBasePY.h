/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InputCodingTable.h"

#include <map>
#include <string>
#include <vector>

class CInputCodingTableBasePY : public IInputCodingTable
{
public:
  CInputCodingTableBasePY();
  ~CInputCodingTableBasePY() override = default;

  bool GetWordListPage(const std::string& strCode, bool isFirstPage) override;
  std::vector<std::wstring> GetResponse(int) override;

private:
  std::vector<std::wstring> m_words;
};
