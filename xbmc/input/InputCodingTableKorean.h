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

class CInputCodingTableKorean : public IInputCodingTable
{
public:
  CInputCodingTableKorean();
  ~CInputCodingTableKorean() override = default;

  bool GetWordListPage(const std::string& strCode, bool isFirstPage) override;
  std::vector<std::wstring> GetResponse(int) override;

  void SetTextPrev(const std::string& strTextPrev) override;
  std::string ConvertString(const std::string& strCode) override;
  int GetType() override { return TYPE_CONVERT_STRING; }

protected:
  int MergeCode(int choseong, int jungseong, int jongseong);
  std::wstring InputToKorean(const std::wstring& input);

private:
  std::vector<std::wstring> m_words;
  std::string m_strTextPrev;
};
