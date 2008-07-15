/*
* UPnP Support for XBMC
* Copyright (c) 2006 c0diq (Sylvain Rebaud)
* Portions Copyright (c) by the authors of libPlatinum
*
* http://www.plutinosoft.com/blog/category/platinum/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#pragma once

#include "IDirectory.h"
#include "VirtualPathDirectory.h"
#include "NptStrings.h"

namespace DIRECTORY
{
/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory class
+---------------------------------------------------------------------*/
class CUPnPVirtualPathDirectory : public CVirtualPathDirectory 
{
public:

    // class methods
    static bool FindSourcePath(const char* share_name, const char* path, bool begin = false);
    static bool SplitPath(const char* object_id, NPT_String& share_name, NPT_String& path);

    // CVirtualPathDirectory overrides
    bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    //bool GetMatchingSource(const CStdString &strPath, CMediaSource& share);
    
    bool GetMatchingSource(const CStdString &strPath, CMediaSource& share, std::vector<CStdString>& paths);
};
}
