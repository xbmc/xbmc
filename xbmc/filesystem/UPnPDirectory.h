/*
 * UPnP Support for XBMC
 *      Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *
 *      Copyright (C) 2010-2013 Team XBMC
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
#pragma once

#include "IDirectory.h"

class CFileItem;
class CURL;

namespace XFILE
{
class CUPnPDirectory :  public IDirectory
{
public:
    CUPnPDirectory(void) {}
    virtual ~CUPnPDirectory(void) {}

    // IDirectory methods
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool AllowAll() const { return true; }

    // class methods
    static const char* GetFriendlyName(const CURL& url);
    static bool        GetResource(const CURL &path, CFileItem& item);
};
}
