#pragma once
/*
 *      Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
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
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define BOOL XBMC_BOOL
#include "FileItem.h"
#include "threads/CriticalSection.h"
#undef BOOL

class CTVOSTopShelf
{
 public:
  CTVOSTopShelf();
  ~CTVOSTopShelf();
  static CTVOSTopShelf &GetInstance();
  void               RunTopShelf();
  void               SetTopShelfItems(CFileItemList& movies, CFileItemList& tv);
  void               HandleTopShelfUrl(const std::string& url, const bool run);
  
 private:
  static std::string m_url;
  static bool        m_handleUrl;
};
