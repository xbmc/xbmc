#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "BackgroundInfoLoader.h"
#include "utils/StdString.h"

class CPictureInfoLoader : public CBackgroundInfoLoader
{
public:
  CPictureInfoLoader();
  virtual ~CPictureInfoLoader();

  void UseCacheOnHD(const CStdString& strFileName);
  virtual bool LoadItem(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
protected:
  CFileItemList* m_mapFileItems;
  unsigned int m_tagReads;
  bool m_loadTags;
};

