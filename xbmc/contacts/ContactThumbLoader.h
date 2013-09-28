#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include "ThumbLoader.h"
#include "FileItem.h"

class CContactFileItem;
class CContactDatabase;

class CContactFileItem : public CFileItem
{
public:
  bool HasContactInfoTag()
  {
    return true;
  }
  CContactFileItem *GetContactInfoTag()
  {
    return NULL;
  }
  std::string GetType()
  {
    return "";
  }
  
};

class CContactThumbLoader : public CThumbLoader
{
public:
  CContactThumbLoader();
  virtual ~CContactThumbLoader();
  
  virtual void Initialize();
  virtual void Deinitialize();
  
  virtual bool LoadItem(CContactFileItem* pItem);
  
  /*! \brief helper function to fill the art for a video library item
   \param item a video CContactFileItem
   \return true if we fill art, false otherwise
   */
  virtual bool FillLibraryArt(CContactFileItem &item);
  
  /*! \brief Fill the thumb of a music file/folder item
   First uses a cached thumb from a previous run, then checks for a local thumb
   and caches it for the next run
   \param item the CContactFileItem object to fill
   \return true if we fill the thumb, false otherwise
   */
  static bool FillThumb(CContactFileItem &item, bool folderThumbs = true);
  
protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  
  CContactDatabase *m_database;
  typedef std::map<int, std::map<std::string, std::string> > ArtCache;
  ArtCache m_albumArt;
};
