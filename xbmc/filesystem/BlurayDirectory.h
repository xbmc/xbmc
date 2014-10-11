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

#include "Directory.h"
#include "FileItem.h"
#include "URL.h"

class  DllLibbluray;
typedef struct bluray BLURAY;
typedef struct bd_title_info BLURAY_TITLE_INFO;

namespace XFILE
{

class CBlurayDirectory: public XFILE::IDirectory
{
public:
  CBlurayDirectory();
  virtual ~CBlurayDirectory();
  virtual bool GetDirectory(const CURL& url, CFileItemList &items);

private:

  void         Dispose();
  void         GetRoot  (CFileItemList &items);
  void         GetTitles(bool main, CFileItemList &items);
  CFileItemPtr GetTitle(const BLURAY_TITLE_INFO* title, const std::string& label);
  CURL         GetUnderlyingCURL(const CURL& url);
  CURL          m_url;
  DllLibbluray* m_dll;
  BLURAY*       m_bd;
};

}
