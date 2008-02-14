/*****************************************************************
|
|   Platinum - Synchronous Media Browser
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_SYNC_MEDIA_BROWSER_
#define _PLT_SYNC_MEDIA_BROWSER_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltCtrlPoint.h"
#include "PltMediaBrowser.h"
#include "PltMediaCache.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef NPT_Map<NPT_String, PLT_DeviceDataReference>         PLT_DeviceMap;
typedef NPT_Map<NPT_String, PLT_DeviceDataReference>::Entry  PLT_DeviceMapEntry;

typedef struct PLT_BrowseData {
    NPT_SharedVariable          shared_var;
    NPT_Result                  res;
    PLT_BrowseInfo              info;
} PLT_BrowseData;

typedef NPT_Reference<PLT_BrowseData> PLT_BrowseDataReference;

/*----------------------------------------------------------------------
|   PLT_DeviceFinder
+---------------------------------------------------------------------*/
class PLT_DeviceFinder
{
public:
    // methods
    PLT_DeviceFinder(const char* ip) : m_IP(ip) {}

    bool operator()(const PLT_DeviceMapEntry* const& entry) const {
        PLT_DeviceDataReference device = entry->GetValue();
        return (device->GetURLBase().GetHost() == m_IP);
    }

private:
    // members
    NPT_String m_IP;
};

/*----------------------------------------------------------------------
|   PLT_DeviceFinderByUUID
+---------------------------------------------------------------------*/
class PLT_DeviceFinderByUUID
{
public:
    // methods
    PLT_DeviceFinderByUUID(const char* uuid) : m_UUID(uuid) {}

    bool operator()(const PLT_DeviceMapEntry* const& entry) const {
        PLT_DeviceDataReference device = entry->GetValue();
        return device->GetUUID() == m_UUID;
    }

private:
    // members
    NPT_String m_UUID;
};

/*----------------------------------------------------------------------
|   PLT_MediaContainerListener
+---------------------------------------------------------------------*/
class PLT_MediaContainerChangesListener
{
public:
    virtual ~PLT_MediaContainerChangesListener() {}
    virtual void OnContainerChanged(PLT_DeviceDataReference& device, const char* item_id, const char* update_id) = 0;
};

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser
+---------------------------------------------------------------------*/
class PLT_SyncMediaBrowser : public PLT_MediaBrowserListener
{
public:
    PLT_SyncMediaBrowser(PLT_CtrlPointReference& ctrlPoint, bool use_cache = false, PLT_MediaContainerChangesListener* listener = NULL);
    virtual ~PLT_SyncMediaBrowser();

    // PLT_MediaBrowserListener
    virtual void OnMSAddedRemoved(PLT_DeviceDataReference& device, int added);
    virtual void OnMSStateVariablesChanged(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars);
    virtual void OnMSBrowseResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_BrowseInfo* info, void* userdata);

    NPT_Result Browse(PLT_DeviceDataReference& device, const char* id, PLT_MediaObjectListReference& list);

    const NPT_Lock<PLT_DeviceMap>& GetMediaServers() const { return m_MediaServers; }

protected:
    NPT_Result Browse(PLT_BrowseDataReference& browse_data,
                      PLT_DeviceDataReference& device, 
                      const char*              object_id,
                      NPT_Int32                index, 
                      NPT_Int32                count,
                      bool                     browse_metadata = false,
                      const char*              filter = "*", 
                      const char*              sort = "");
private:
    NPT_Result Find(const char* ip, PLT_DeviceDataReference& device);
    NPT_Result WaitForResponse(NPT_SharedVariable& shared_var);

private:
    NPT_Lock<PLT_DeviceMap>              m_MediaServers;
    PLT_MediaBrowser*                    m_MediaBrowser;
    PLT_MediaContainerChangesListener*   m_ContainerListener;
    bool                                 m_UseCache;
    PLT_MediaCache                       m_Cache;
};

#endif /* _PLT_SYNC_MEDIA_BROWSER_ */

