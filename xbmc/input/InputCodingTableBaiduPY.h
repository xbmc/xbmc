/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InputCodingTable.h"
#include "threads/Thread.h"

#include <deque>
#include <map>
#include <string>
#include <vector>

class CInputCodingTableBaiduPY : public IInputCodingTable, public CThread
{
public:
  explicit CInputCodingTableBaiduPY(const std::string& strUrl);
  ~CInputCodingTableBaiduPY() override = default;

  void Initialize() override;
  void Deinitialize() override;
  bool IsInitialized() const override;
  bool GetWordListPage(const std::string& strCode, bool isFirstPage) override;
  void Process() override;

  std::vector<std::wstring> GetResponse(int response) override;
private:
  std::wstring UnicodeToWString(const std::string& unicode);
  void HandleResponse(const std::string& strCode, const std::string& response);

  std::string m_url;
  std::string m_code;
  int m_messageCounter;
  int m_api_begin; // baidu api begin num
  int m_api_end;   // baidu api end num
  bool m_api_nomore;
  bool m_initialized;

  std::deque<std::string> m_work;
  std::map<int, std::vector<std::wstring>> m_responses;
  CEvent            m_Event;
  CCriticalSection  m_CS;
};
