/*****************************************************************
|
|   Platinum - AV Media Server Device
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
#include "PltUPnP.h"
#include "PltMediaServer.h"
#include "PltMediaItem.h"
#include "PltService.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDidl.h"
#include "PltMetadataHandler.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server")

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
extern NPT_UInt8 MS_ConnectionManagerSCPD[];
extern NPT_UInt8 MS_ContentDirectorySCPD[];
extern NPT_UInt8 MS_ContentDirectorywSearchSCPD[];

const char* BrowseFlagsStr[] = {
    "BrowseMetadata",
    "BrowseDirectChildren"
};

/*----------------------------------------------------------------------
|   PLT_MediaServer::PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::PLT_MediaServer(const char*  friendly_name, 
                                 bool         show_ip     /* = false */, 
                                 const char*  uuid        /* = NULL */, 
                                 NPT_UInt16   port        /* = 0 */,
                                 bool         port_rebind /* = false */) :	
    PLT_DeviceHost("/DeviceDescription.xml", 
                   uuid, 
                   "urn:schemas-upnp-org:device:MediaServer:1", 
                   friendly_name, 
                   show_ip, 
                   port,
                   port_rebind)
{
    m_ModelDescription = "Plutinosoft AV Media Server Device";
    m_ModelName        = "AV Media Server Device";
    m_ModelNumber      = "1.0";
    m_ModelURL         = "http://www.plutinosoft.com/blog/projects/platinum";
    m_DlnaDoc          = "DMS-1.50";
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::~PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::~PLT_MediaServer()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::SetupServices(PLT_DeviceData& data)
{
    PLT_Service* service;

    {
        service = new PLT_Service(
            &data,
            "urn:schemas-upnp-org:service:ContentDirectory:1", 
            "urn:upnp-org:serviceId:ContentDirectory");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) MS_ContentDirectorywSearchSCPD));
        NPT_CHECK_FATAL(service->InitURLs("ContentDirectory", data.GetUUID()));
        NPT_CHECK_FATAL(data.AddService(service));
        
        service->SetStateVariable("ContainerUpdateIDs", "0");
        service->SetStateVariableRate("ContainerUpdateIDs", NPT_TimeInterval(2, 0));
        service->SetStateVariable("SystemUpdateID", "0");
        service->SetStateVariableRate("SystemUpdateID", NPT_TimeInterval(2, 0));
        service->SetStateVariable("SearchCapability", "upnp:class");
        service->SetStateVariable("SortCapability", "");
    }

    {
        service = new PLT_Service(
            &data,
            "urn:schemas-upnp-org:service:ConnectionManager:1", 
            "urn:upnp-org:serviceId:ConnectionManager");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) MS_ConnectionManagerSCPD));
        NPT_CHECK_FATAL(service->InitURLs("ConnectionManager", data.GetUUID()));
        NPT_CHECK_FATAL(data.AddService(service));
        
        service->SetStateVariable("CurrentConnectionIDs", "0");
        service->SetStateVariable("SinkProtocolInfo", "");
        service->SetStateVariable("SourceProtocolInfo", "http-get:*:*:*");
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnAction(PLT_ActionReference&          action, 
                          const PLT_HttpRequestContext& context)
{
    /* parse the action name */
    NPT_String name = action->GetActionDesc().GetName();
                   
    // ContentDirectory
    if (name.Compare("Browse", true) == 0) {
        return OnBrowse(action, context);
    }
    if (name.Compare("Search", true) == 0) {
        return OnSearch(action, context);
    }
    if (name.Compare("GetSystemUpdateID", true) == 0) {
        return OnGetSystemUpdateID(action, context);
    }
    if (name.Compare("GetSortCapabilities", true) == 0) {
        return OnGetSortCapabilities(action, context);
    }  
    if (name.Compare("GetSearchCapabilities", true) == 0) {
        return OnGetSearchCapabilities(action, context);
    }  

    // ConnectionMananger
    if (name.Compare("GetCurrentConnectionIDs", true) == 0) {
        return OnGetCurrentConnectionIDs(action, context);
    }
    if (name.Compare("GetProtocolInfo", true) == 0) {
        return OnGetProtocolInfo(action, context);
    }    
    if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnGetCurrentConnectionInfo(action, context);
    }

    action->SetError(401,"No Such Action.");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionIDs
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnGetCurrentConnectionIDs(PLT_ActionReference&          action,
                                           const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    return action->SetArgumentsOutFromStateVariable();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetProtocolInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnGetProtocolInfo(PLT_ActionReference&          action,
                                   const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    return action->SetArgumentsOutFromStateVariable();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetCurrentConnectionInfo(PLT_ActionReference&          action, 
                                            const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    if (NPT_FAILED(action->VerifyArgumentValue("ConnectionID", "0"))) {
        action->SetError(706,"No Such Connection.");
        return NPT_FAILURE;
    }

    if (NPT_FAILED(action->SetArgumentValue("RcsID", "-1"))){
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("AVTransportID", "-1"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("ProtocolInfo", "http-get:*:*:*"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionManager", "/"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionID", "-1"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Direction", "Output"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Status", "Unknown"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSortCapabilities
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnGetSortCapabilities(PLT_ActionReference&          action,
                                       const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    return action->SetArgumentsOutFromStateVariable();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSearchCapabilities
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnGetSearchCapabilities(PLT_ActionReference&          action, 
                                         const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    return action->SetArgumentsOutFromStateVariable();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSystemUpdateID
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnGetSystemUpdateID(PLT_ActionReference&          action, 
                                     const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    return action->SetArgumentsOutFromStateVariable();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::GetBrowseFlag
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::GetBrowseFlag(const char* str, BrowseFlags& flag) 
{
    if (NPT_String::Compare(str, BrowseFlagsStr[0], true) == 0) {
        flag = BROWSEMETADATA;
        return NPT_SUCCESS;
    }
    if (NPT_String::Compare(str, BrowseFlagsStr[1], true) == 0) {
        flag = BROWSEDIRECTCHILDREN;
        return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::ParseSort
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::ParseSort(const NPT_String& sort, NPT_List<NPT_String>& list)
{
    // reset output params first
    list.Clear();
    
    // easy out
    if (sort.GetLength() == 0 || sort == "*") return NPT_SUCCESS;
    
    list = sort.Split(",");
    
    // verify each property has a namespace
    NPT_List<NPT_String>::Iterator property = list.GetFirstItem();
    while (property) {
        NPT_List<NPT_String> parsed_property = (*property).Split(":");
        if (parsed_property.GetItemCount() != 2 || 
            (!(*property).StartsWith("-") && !(*property).StartsWith("+"))) {
            NPT_LOG_WARNING_1("Invalid SortCriteria property %s", (*property).GetChars());
            return NPT_FAILURE;
        }
        property++;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnBrowse(PLT_ActionReference&          action, 
                          const PLT_HttpRequestContext& context)
{
    NPT_Result res;
    NPT_String object_id;
    NPT_String browse_flag_val;    
    NPT_String filter;
    NPT_String start;
    NPT_String count;
    NPT_String sort;
    NPT_List<NPT_String> sort_list;

    if (NPT_FAILED(action->GetArgumentValue("ObjectId", object_id)) || 
        NPT_FAILED(action->GetArgumentValue("BrowseFlag",  browse_flag_val)) || 
        NPT_FAILED(action->GetArgumentValue("Filter",  filter)) || 
        NPT_FAILED(action->GetArgumentValue("StartingIndex",  start)) || 
        NPT_FAILED(action->GetArgumentValue("RequestedCount",  count)) || 
        NPT_FAILED(action->GetArgumentValue("SortCriteria",  sort))) {
        NPT_LOG_WARNING("Missing arguments");
        action->SetError(402, "Invalid args");
        return NPT_SUCCESS;
    }

    /* extract flag */
    BrowseFlags flag;
    if (NPT_FAILED(GetBrowseFlag(browse_flag_val, flag))) {
        /* error */
        NPT_LOG_WARNING_1("BrowseFlag value not allowed (%s)", (const char*)browse_flag_val);
        action->SetError(402, "Invalid args");
        return NPT_SUCCESS;
    }
    
    /* convert index and counts to int */
    NPT_UInt32 starting_index, requested_count;
    if (NPT_FAILED(start.ToInteger(starting_index)) ||
        NPT_FAILED(count.ToInteger(requested_count)) ||
        PLT_Didl::ConvertFilterToMask(filter) == 0) {       
        NPT_LOG_WARNING_3("Invalid arguments (%s, %s, %s)", 
            start.GetChars(), count.GetChars(), filter.GetChars());
        action->SetError(402, "Invalid args");
        return NPT_FAILURE;
    }
    
    /* parse sort criteria */
    if (NPT_FAILED(ParseSort(sort, sort_list))) {
        NPT_LOG_WARNING_1("Unsupported or invalid sort criteria error (%s)", 
            sort.GetChars());
        action->SetError(709, "Unsupported or invalid sort criteria error");
        return NPT_FAILURE;
    }
    
    NPT_LOG_INFO_6("Received %s from %s for id = %s with filter = %s, start = %d, count = %d", 
                   (const char*)browse_flag_val, 
                   (const char*)context.GetRemoteAddress().GetIpAddress().ToString(),
                   (const char*)object_id,
                   (const char*)filter,
                   starting_index,
                   requested_count);

    /* Invoke the browse function */
    if (flag == BROWSEMETADATA) {
        res = OnBrowseMetadata(
            action, 
            object_id, 
            filter, 
            starting_index, 
            requested_count, 
            sort_list, 
            context);
    } else {
        res = OnBrowseDirectChildren(
            action, 
            object_id, 
            filter, 
            starting_index, 
            requested_count, 
            sort_list, 
            context);
    }

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnSearch
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnSearch(PLT_ActionReference&          action, 
                          const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    NPT_Result res;
    NPT_String container_id;
    NPT_String search;
	NPT_String filter;
    NPT_String start;
    NPT_String count;
    NPT_String sort;
    NPT_List<NPT_String> sort_list;

    if (NPT_FAILED(action->GetArgumentValue("ContainerId", container_id)) ||
        NPT_FAILED(action->GetArgumentValue("SearchCriteria", search)) || 
		NPT_FAILED(action->GetArgumentValue("Filter",  filter)) ||
        NPT_FAILED(action->GetArgumentValue("StartingIndex",  start)) || 
        NPT_FAILED(action->GetArgumentValue("RequestedCount",  count)) || 
        NPT_FAILED(action->GetArgumentValue("SortCriteria",  sort))) {
        NPT_LOG_WARNING("Missing arguments");
        action->SetError(402, "Invalid args");
        return NPT_SUCCESS;
    }
    
    /* convert index and counts to int */
    NPT_UInt32 starting_index, requested_count;
    if (NPT_FAILED(start.ToInteger(starting_index)) ||
        NPT_FAILED(count.ToInteger(requested_count))) {       
        NPT_LOG_WARNING_2("Invalid arguments (%s, %s)", 
            start.GetChars(), count.GetChars());
        action->SetError(402, "Invalid args");
        return NPT_FAILURE;
    }
    
    /* parse sort criteria */
    if (NPT_FAILED(ParseSort(sort, sort_list))) {
        NPT_LOG_WARNING_1("Unsupported or invalid sort criteria error (%s)", 
            sort.GetChars());
        action->SetError(709, "Unsupported or invalid sort criteria error");
        return NPT_FAILURE;
    }
    
    NPT_LOG_INFO_5("Received Search from %s for id = %s with search = %s, start = %d, count = %d", 
                   (const char*)context.GetRemoteAddress().GetIpAddress().ToString(),
                   (const char*)container_id,
                   (const char*)search,
                   starting_index,
                   requested_count);
                       
    if (search.IsEmpty() || search == "*") {
        res = OnBrowseDirectChildren(
            action, 
            container_id,
			filter,
            starting_index, 
            requested_count, 
            sort_list, 
            context);
    } else {
        res = OnSearchContainer(
            action, 
            container_id, 
            search, 
			filter,
            starting_index, 
            requested_count, 
            sort_list,
            context);
    }

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseMetadata(PLT_ActionReference&          /* action */, 
                                  const char*                   /* object_id */, 
                                  const char*                   /* filter */,
                                  NPT_UInt32                    /* starting_index */,
                                  NPT_UInt32                    /* requested_count */,
                                  const NPT_List<NPT_String>&   /* sort_criteria */,
                                  const PLT_HttpRequestContext& /* context */)
{ 
    return NPT_ERROR_NOT_IMPLEMENTED; 
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseDirectChildren(PLT_ActionReference&          /* action */, 
                                        const char*                   /* object_id */, 
                                        const char*                   /* filter */,
                                        NPT_UInt32                    /* starting_index */,
                                        NPT_UInt32                    /* requested_count */,
                                        const NPT_List<NPT_String>&   /* sort_criteria */,
                                        const PLT_HttpRequestContext& /* context */) 
{ 
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnSearchContainer
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnSearchContainer(PLT_ActionReference&          /* action */, 
                                   const char*                   /* object_id */, 
                                   const char*                   /* search_criteria */,
								   const char*                   /* filter */,
                                   NPT_UInt32                    /* starting_index */,
                                   NPT_UInt32                    /* requested_count */,
                                   const NPT_List<NPT_String>&   /* sort_criteria */,
                                   const PLT_HttpRequestContext& /* context */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
