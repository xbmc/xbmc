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

#ifndef CRSSDIRECTORY_H_
#define CRSSDIRECTORY_H_

#include "IFileDirectory.h"
#include "FileItem.h"

namespace XFILE
{
  class CRSSDirectory : public IFileDirectory
  {
  public:
    CRSSDirectory();
    virtual ~CRSSDirectory();
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Exists(const CURL& url);
    virtual bool AllowAll() const { return true; }
    virtual bool ContainsFiles(const CURL& url);
    virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; };
  protected:
    // key is path, value is cache invalidation date
    static std::map<std::string,CDateTime> m_cache;
    static CCriticalSection m_section;
  };
}

#endif /*CRSSDIRECTORY_H_*/
