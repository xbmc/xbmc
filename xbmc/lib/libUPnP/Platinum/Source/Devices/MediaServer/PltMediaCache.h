/*****************************************************************
|
|   Platinum - AV Media Cache
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

#ifndef _PLT_MEDIA_CACHE_H_
#define _PLT_MEDIA_CACHE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaItem.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   typedefs
+---------------------------------------------------------------------*/
typedef NPT_Map<NPT_String, PLT_MediaObjectListReference>::Entry PLT_MediaCacheEntry;

/*----------------------------------------------------------------------
|   PLT_MediaCache class
+---------------------------------------------------------------------*/
class PLT_MediaCache
{
public:
    PLT_MediaCache();
    virtual ~PLT_MediaCache();

    NPT_Result Put(const char* device_uuid, const char* item_id, PLT_MediaObjectListReference& list);
    NPT_Result Get(const char* device_uuid, const char* item_id, PLT_MediaObjectListReference& list);
    NPT_Result Clear(const char* device_uuid, const char* item_id);
    NPT_Result Clear(const char* device_uuid = NULL);

private:
    NPT_String GenerateKey(const char* device_uuid, const char* item_id);

private:
    NPT_Mutex m_Mutex;
    NPT_Map<NPT_String, PLT_MediaObjectListReference> m_Items;
};

#endif /* _PLT_MEDIA_CACHE_H_ */
