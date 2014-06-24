#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "PlayList.h"

namespace PLAYLIST
{
class CPlayListPLS :
      public CPlayList
{
public:
  CPlayListPLS(void);
  virtual ~CPlayListPLS(void);
  virtual bool Load(const std::string& strFileName);
  virtual void Save(const std::string& strFileName) const;
  virtual bool Resize(std::vector<int>::size_type newSize);
};

class CPlayListASX : public CPlayList
{
public:
  virtual bool LoadData(std::istream &stream);
protected:
  bool LoadAsxIniInfo(std::istream &stream);
};

class CPlayListRAM : public CPlayList
{
public:
  virtual bool LoadData(std::istream &stream);
};


}
