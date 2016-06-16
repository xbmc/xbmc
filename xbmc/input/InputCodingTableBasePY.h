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

#include "InputCodingTable.h"
#include <map>
#include <string>
#include <vector>

class CInputCodingTableBasePY : public IInputCodingTable
{
public:
  CInputCodingTableBasePY();
  virtual ~CInputCodingTableBasePY() {}

  virtual bool GetWordListPage(const std::string& strCode, bool isFirstPage);
  virtual std::vector<std::wstring> GetResponse(int);
private:
  std::vector<std::wstring> m_words;
};
