#pragma once

/*
*      Copyright (C) 2005-2013 Team Kodi
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

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "InputCodingTable.h"
#include "threads/Thread.h"

class CInputCodingTableBaiduPY : public IInputCodingTable, public CThread
{
public:
  CInputCodingTableBaiduPY(const std::string& strUrl);
  virtual ~CInputCodingTableBaiduPY() {}

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;
  virtual bool GetWordListPage(const std::string& strCode, bool isFirstPage) override;
  virtual void Process() override;

  virtual std::vector<std::wstring> GetResponse(int response) override;
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
