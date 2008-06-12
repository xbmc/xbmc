/*****************************************************************
|
|   Platinum - AV Media Cache
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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

    NPT_Result Put(PLT_DeviceDataReference& device, const char* item_id, PLT_MediaObjectListReference& list);
    NPT_Result Get(PLT_DeviceDataReference& device, const char* item_id, PLT_MediaObjectListReference& list);
    NPT_Result Clear(PLT_DeviceDataReference& device, const char* item_id);
    NPT_Result Clear(PLT_DeviceData* device = NULL);

private:
    NPT_String GenerateKey(const char* device_uuid, const char* item_id);

private:
    NPT_Map<NPT_String, PLT_MediaObjectListReference> m_Items;
};

#endif /* _PLT_MEDIA_CACHE_H_ */
