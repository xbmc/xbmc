#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "DbUrl.h"

class CVideoDbUrl : public CDbUrl
{
public:
  CVideoDbUrl();
  virtual ~CVideoDbUrl();

  const std::string& GetItemType() const { return m_itemType; }

protected:
  virtual bool parse();
  virtual bool validateOption(const std::string &key, const CVariant &value);

private:
  std::string m_itemType;
};
