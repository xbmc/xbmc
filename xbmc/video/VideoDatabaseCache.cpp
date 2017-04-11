/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoDatabaseCache.h"
#include "VideoInfoTag.h"

CVideoDatabaseCache::CVideoDatabaseCache()
{

}

CVideoDatabaseCache::~CVideoDatabaseCache()
{

}

void CVideoDatabaseCache::addItem(long id, std::shared_ptr<CVideoInfoTag>& item, int getDetails)
{
  CVideoDatabaseCacheItem cacheItem;
  cacheItem.m_getDetails = getDetails;
  cacheItem.m_item = item;
  
  m_cacheMap.insert(std::make_pair(id, cacheItem));
}

std::shared_ptr<CVideoInfoTag> CVideoDatabaseCache::getItem(long id, int getDetails)
{
  tCacheMap ::iterator it = m_cacheMap.find(id);

  if (it != m_cacheMap.end())
  {
    // If we do not have enough details, delete the item
    if (it->second.m_getDetails < getDetails)
    {
      m_cacheMap.erase(it);
      return nullptr;
    }
    
    return it->second.m_item;
  }

  return nullptr;
}
