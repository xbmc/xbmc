#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#include <string>
#include <vector>

#include "PlayList.h"

namespace PLAYLIST
{
class CPlayListPLS :
      public CPlayList
{
public:
  CPlayListPLS(void);
  ~CPlayListPLS(void) override;
  bool Load(const std::string& strFileName) override;
  void Save(const std::string& strFileName) const override;
  virtual bool Resize(std::vector<int>::size_type newSize);
};

class CPlayListASX : public CPlayList
{
public:
  bool LoadData(std::istream &stream) override;
protected:
  bool LoadAsxIniInfo(std::istream &stream);
};

class CPlayListRAM : public CPlayList
{
public:
  bool LoadData(std::istream &stream) override;
};


}
