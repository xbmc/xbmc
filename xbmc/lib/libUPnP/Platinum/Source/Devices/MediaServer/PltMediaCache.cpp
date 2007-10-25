/*****************************************************************
|
|   Platinum - AV Media Cache
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
PLT_MediaCache::Clear(PLT_DeviceDataReference& device, 
                      const char*              item_id)
{
    NPT_String key = GenerateKey(device->GetUUID(), item_id);
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
PLT_MediaCache::Clear(PLT_DeviceData* device)
{
    if (!device) return m_Items.Clear();

    NPT_String key = GenerateKey(device->GetUUID(), "");
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
PLT_MediaCache::Put(PLT_DeviceDataReference&      device, 
                    const char*                   item_id, 
                    PLT_MediaObjectListReference& list)
{
    NPT_String key = GenerateKey(device->GetUUID(), item_id);
    if (key.GetLength() == 0) return NPT_ERROR_INVALID_PARAMETERS;

    m_Items.Erase(key);
    return m_Items.Put(key, list);
}

/*----------------------------------------------------------------------
|   PLT_MediaCache::Get
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaCache::Get(PLT_DeviceDataReference&      device, 
                    const char*                   item_id, 
                    PLT_MediaObjectListReference& list)
{
    NPT_String key = GenerateKey(device->GetUUID(), item_id);
    if (key.GetLength() == 0) return NPT_ERROR_INVALID_PARAMETERS;
    
    PLT_MediaObjectListReference* val = NULL;
    NPT_CHECK(m_Items.Get(key, val));

    list = *val;
    return NPT_SUCCESS;
}

