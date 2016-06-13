#pragma once

/*
*      Copyright (C) 2005-2015 Team Kodi
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

#include "InputCodingTable.h"
#include <map>
#include <string>
#include <vector>

class CInputCodingTableKorean : public IInputCodingTable
{
public:
  CInputCodingTableKorean();
  virtual ~CInputCodingTableKorean() {}

  virtual bool GetWordListPage(const std::string& strCode, bool isFirstPage);
  virtual std::vector<std::wstring> GetResponse(int);

  virtual void SetTextPrev(const std::string& strTextPrev);
  virtual std::string ConvertString(const std::string& strCode);
  virtual int GetType() { return TYPE_CONVERT_STRING;  }

protected:
  int MergeCode(int choseong, int jungseong, int jongseong);
  std::wstring InputToKorean(const std::wstring& input);

private:
  std::vector<std::wstring> m_words;
  std::string m_strTextPrev;
};
