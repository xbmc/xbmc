#pragma once

/*
*      Copyright (C) 2005-2013 Team Kodi
*      http://kodi.tv
*
*  Kodi is free software: you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  Kodi is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
  ~CInputCodingTableBasePY() override = default;

  bool GetWordListPage(const std::string& strCode, bool isFirstPage) override;
  std::vector<std::wstring> GetResponse(int) override;
private:
  std::vector<std::wstring> m_words;
};
