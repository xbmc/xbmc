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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMediaCache.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.cache")

/*----------------------------------------------------------------------
|   PLT_MediaCache::PLT_MediaCache
+---------------------------------------------------------------------*/
PLT_MediaCache::PLT_MediaCache()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::~PLT_MediaCache
+---------------------------------------------------------------------*/
PLT_MediaCache::~PLT_MediaCache()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::GenerateKey
+---------------------------------------------------------------------*/
NPT_String
PLT_MediaCache::GenerateKey(const char* device_uuid, 
                            const char* item_id)
{
    NPT_String key = "upnp://";
    key += device_uuid;
    key += "/";
    key += item_id;

    return key;
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::Clear
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaCache::Clear(const char* device_uuid, 
                      const char* item_id)
{
    NPT_AutoLock lock(m_Mutex);
    
    NPT_String key = GenerateKey(device_uuid, item_id);
    if (key.GetLength() == 0) return NPT_ERROR_INVALID_PARAMETERS;

    NPT_List<PLT_MediaCacheEntry*>::Iterator entries = m_Items.GetEntries().GetFirstItem();
    NPT_List<PLT_MediaCacheEntry*>::Iterator entry;
    while (entries) {
        entry = entries++;
        if ((*entry)->GetKey() == (key)) {
            m_Items.Erase(key);
            return NPT_SUCCESS;
        }
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::Clear
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaCache::Clear(const char* device_uuid)
{
    NPT_AutoLock lock(m_Mutex);

    if (!device_uuid) return m_Items.Clear();

    NPT_String key = GenerateKey(device_uuid, "");
    NPT_List<PLT_MediaCacheEntry*>::Iterator entries = m_Items.GetEntries().GetFirstItem();
    NPT_List<PLT_MediaCacheEntry*>::Iterator entry;
    while (entries) {
        entry = entries++;
        NPT_String entry_key = (*entry)->GetKey();
        if (entry_key.StartsWith(key)) {
            m_Items.Erase(entry_key);
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::Put
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaCache::Put(const char*                   device_uuid,
                    const char*                   item_id, 
                    PLT_MediaObjectListReference& list)
{
    NPT_AutoLock lock(m_Mutex);

    NPT_String key = GenerateKey(device_uuid, item_id);
    if (key.GetLength() == 0) return NPT_ERROR_INVALID_PARAMETERS;

    m_Items.Erase(key);
    return m_Items.Put(key, list);
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::Get
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaCache::Get(const char*                   device_uuid,
                    const char*                   item_id, 
                    PLT_MediaObjectListReference& list)
{
    NPT_AutoLock lock(m_Mutex);

    NPT_String key = GenerateKey(device_uuid, item_id);
    if (key.GetLength() == 0) return NPT_ERROR_INVALID_PARAMETERS;
    
    PLT_MediaObjectListReference* val = NULL;
    NPT_CHECK_FINE(m_Items.Get(key, val));

    list = *val;
    return NPT_SUCCESS;
}

