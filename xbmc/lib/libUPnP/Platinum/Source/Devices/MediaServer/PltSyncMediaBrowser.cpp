/*****************************************************************
|
|   Platinum - Synchronous Media Browser
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#include "PltSyncMediaBrowser.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.syncbrowser")

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::PLT_SyncMediaBrowser
+---------------------------------------------------------------------*/
PLT_SyncMediaBrowser::PLT_SyncMediaBrowser(PLT_CtrlPointReference& ctrlPoint,
                                           bool                    use_cache /* = false */, 
                                           PLT_MediaContainerChangesListener* listener /* = NULL */) :
    m_ContainerListener(listener),
    m_UseCache(use_cache)
{
    m_MediaBrowser = new PLT_MediaBrowser(ctrlPoint, this);
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::~PLT_SyncMediaBrowser
+---------------------------------------------------------------------*/
PLT_SyncMediaBrowser::~PLT_SyncMediaBrowser()
{
    delete m_MediaBrowser;
}

/*  Blocks forever waiting for a response from a request
 *  It is expected the request to succeed or to timeout and return an error eventually
 */
/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::WaitForResponse
+---------------------------------------------------------------------*/
NPT_Result
PLT_SyncMediaBrowser::WaitForResponse(NPT_SharedVariable& shared_var)
{
    return shared_var.WaitUntilEquals(1, 30000);
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::OnMSAddedRemoved
+---------------------------------------------------------------------*/
void
PLT_SyncMediaBrowser::OnMSAddedRemoved(PLT_DeviceDataReference& device, int added)
{
    NPT_String uuid = device->GetUUID();

    if (added) {
        // test if it's a media server
        m_MediaServers.Lock();
        PLT_Service* service;
        if (NPT_SUCCEEDED(device->FindServiceByType("urn:schemas-upnp-org:service:ContentDirectory:1", service))) {
            m_MediaServers.Put(uuid, device);
        }
        m_MediaServers.Unlock();
    } else { /* removed */
        // Remove from our list of servers first if found
        m_MediaServers.Lock();
        m_MediaServers.Erase(uuid);
        m_MediaServers.Unlock();

        // clear cache for that device
        m_Cache.Clear(device.AsPointer());
    }
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::Find
+---------------------------------------------------------------------*/
NPT_Result
PLT_SyncMediaBrowser::Find(const char* ip, PLT_DeviceDataReference& device)
{
    NPT_AutoLock lock(m_MediaServers);
    const NPT_List<PLT_DeviceMapEntry*>::Iterator it = m_MediaServers.GetEntries().Find(PLT_DeviceFinder(ip));
    if (it) {
        device = (*it)->GetValue();
        return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::OnMSBrowseResult
+---------------------------------------------------------------------*/
void
PLT_SyncMediaBrowser::OnMSBrowseResult(NPT_Result res, PLT_DeviceDataReference& device, PLT_BrowseInfo* info, void* userdata)
{
    NPT_COMPILER_UNUSED(device);

    if (!userdata) return;

    PLT_BrowseDataReference* data = (PLT_BrowseDataReference*) userdata;
    (*data)->res = res;
    if (NPT_SUCCEEDED(res) && info) {
        (*data)->info = *info;
    }
    (*data)->shared_var.SetValue(1);
    delete data;
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::OnMSStateVariablesChanged
+---------------------------------------------------------------------*/
void 
PLT_SyncMediaBrowser::OnMSStateVariablesChanged(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars)
{
    NPT_AutoLock lock(m_MediaServers);
    PLT_DeviceDataReference device;
    const NPT_List<PLT_DeviceMapEntry*>::Iterator it = m_MediaServers.GetEntries().Find(PLT_DeviceFinderByUUID(service->GetDevice()->GetUUID()));
    if (!it) return; // device with this service has gone away

    device = (*it)->GetValue();
    PLT_StateVariable* var = NULL;
    if (NPT_SUCCEEDED(NPT_ContainerFind(*vars, PLT_StateVariableNameFinder("ContainerUpdateIDs"), var))) {
        // variable found, parse value
        NPT_String value = var->GetValue();
        NPT_String item_id, update_id;
        int index;

        while (value.GetLength()) {
            // look for container id
            index = value.Find(',');
            if (index < 0) break;
            item_id = value.Left(index);
            value = value.SubString(index+1);

            // look for update id
            if (value.GetLength()) {
                index = value.Find(',');
                update_id = (index<0)?value:value.Left(index);
                value = (index<0)?"":value.SubString(index+1);

                // clear cache for that device
                if (m_UseCache) m_Cache.Clear(device.AsPointer());

                // notify listener
                if (m_ContainerListener) m_ContainerListener->OnContainerChanged(device, item_id, update_id);
            }       
        }
    }        
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::Browse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_SyncMediaBrowser::Browse(PLT_BrowseDataReference& browse_data,
                             PLT_DeviceDataReference& device, 
                             const char*              object_id, 
                             NPT_Int32                index, 
                             NPT_Int32                count,
                             bool                     browse_metadata,
                             const char*              filter, 
                             const char*              sort)
{
    NPT_Result res;


    {
        // make sure the device is still alive
        NPT_AutoLock lock(m_MediaServers);
        PLT_DeviceDataReference* data;
        if (NPT_FAILED(m_MediaServers.Get(device->GetUUID(), data))) {
            NPT_LOG_WARNING_1("Device (%s) not found in our list!\n", (const char*)device->GetUUID());
            return NPT_FAILURE;
        }
    }

    browse_data->shared_var.SetValue(0);

    // send off the browse packet.  Note that this will
    // not block.  There is a call to WaitForResponse in order
    // to block until the response comes back.
    res = m_MediaBrowser->Browse(device,
        (const char*)object_id,
        index,
        count,
        browse_metadata,
        filter,
        sort,
        new PLT_BrowseDataReference(browse_data));		
    NPT_CHECK_SEVERE(res);

    return WaitForResponse(browse_data->shared_var);
}

/*----------------------------------------------------------------------
|   PLT_SyncMediaBrowser::Browse
+---------------------------------------------------------------------*/
NPT_Result
PLT_SyncMediaBrowser::Browse(PLT_DeviceDataReference&      device, 
                             const char*                   object_id, 
                             PLT_MediaObjectListReference& list)
{
    NPT_Result res = NPT_FAILURE;
    NPT_Int32  index = 0;

    // reset output params
    list = NULL;

    // look into cache first
    if (m_UseCache && NPT_SUCCEEDED(m_Cache.Get(device, object_id, list))) return NPT_SUCCESS;

    do {	
        PLT_BrowseDataReference browse_data(new PLT_BrowseData());
        // send off the browse packet.  Note that this will
        // not block.  There is a call to WaitForResponse in order
        // to block until the response comes back.
        res = Browse(browse_data,
            device,
            (const char*)object_id,
            index,
            1024,
            false,
            "*",
            "");		
        if (NPT_FAILED(res)) 
            break;
        
        if (NPT_FAILED(browse_data->res)) 
            return browse_data->res;

        if (browse_data->info.items->GetItemCount() == 0)
            break;

        if (list.IsNull()) {
            list = browse_data->info.items;
        } else {
            list->Add(*browse_data->info.items);
            // clear the list items so that the data inside is not
            // cleaned up by PLT_MediaItemList dtor since we copied
            // each pointer into the new list.
            browse_data->info.items->Clear();
        }

        // stop now if our list contains exactly what the server said it had
        if (browse_data->info.tm && browse_data->info.tm == list->GetItemCount())
            break;

        // ask for the next chunk of entries
        index = list->GetItemCount();
    } while(1);

    // cache the result
    if (m_UseCache && NPT_SUCCEEDED(res)) {
        m_Cache.Put(device, object_id, list);
    }

    return res;
}
