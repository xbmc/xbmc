/*****************************************************************
|
|   Platinum - AV Media Server Device
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "PltLog.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "PltUPnP.h"
#include "PltMediaServer.h"
#include "PltMediaItem.h"
#include "PltService.h"
#include "PltTaskManager.h"
#include "PltHttpServer.h"
#include "PltDidl.h"
#include "PltMetadataHandler.h"

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
extern NPT_UInt8 MS_ConnectionManagerSCPD[];
extern NPT_UInt8 MS_ContentDirectorySCPD[];

const char* BrowseFlagsStr[] = {
    "BrowseMetadata",
    "BrowseDirectChildren"
};

/*----------------------------------------------------------------------
|   PLT_MediaServer::PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::PLT_MediaServer(const char*  friendly_name, 
                                 bool         show_ip, 
                                 const char*  uuid, 
                                 unsigned int port, 
                                 unsigned int fileserver_port) :	
    PLT_DeviceHost("/", uuid, "urn:schemas-upnp-org:device:MediaServer:1", friendly_name, port),
    m_ShowIP(show_ip)
{
    PLT_Service* service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:ContentDirectory:1", 
        "urn:upnp-org:serviceId:CDS_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) MS_ContentDirectorySCPD))) {
        service->InitURLs("ContentDirectory", m_UUID);
        AddService(service);
        service->SetStateVariable("ContainerUpdateIDs", "0", false);
        service->SetStateVariable("SystemUpdateID", "0", false);
    }

    service = new PLT_Service(
        this,
        "urn:schemas-upnp-org:service:ConnectionManager:1", 
        "urn:upnp-org:serviceId:CMGR_1-0");
    if (NPT_SUCCEEDED(service->SetSCPDXML((const char*) MS_ConnectionManagerSCPD))) {
        service->InitURLs("ConnectionManager", m_UUID);
        AddService(service);
        service->SetStateVariable("CurrentConnectionIDs", "0", false);
        service->SetStateVariable("SinkProtocolInfo", "", false);
        service->SetStateVariable("SourceProtocolInfo", "http-get:*:*:*", false);
    }

    m_FileServerHandler = new PLT_HttpFileServerHandler(this);
    m_FileServer = new PLT_HttpServer(m_FileServerHandler, fileserver_port);
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::~PLT_MediaServer
+---------------------------------------------------------------------*/
PLT_MediaServer::~PLT_MediaServer()
{
    if (m_FileServer) {
        delete m_FileServer;
    }

    if (m_FileServerHandler) {
        delete m_FileServerHandler;
    }
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::Start
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::Start(PLT_TaskManager* task_manager)
{
    NPT_String ip;
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_Result res = NPT_NetworkInterface::GetNetworkInterfaces(if_list);
    if (NPT_SUCCEEDED(res) && if_list.GetItemCount()) {
        ip = (*(*if_list.GetFirstItem())->GetAddresses().GetFirstItem()).GetPrimaryAddress().ToString();
        PLT_Log(PLT_LOG_LEVEL_1, "IP addr: %s\n", (const char*)ip);
    }   
    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());
    
    // update Friendlyname with ip
    if (m_ShowIP && ip.GetLength()) {
        m_FriendlyName += " (" + ip + ")";
    }

    // start our file server
    m_FileServer->Start();

    return PLT_DeviceHost::Start(task_manager);
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::Stop()
{
    m_FileServer->Stop();

    return PLT_DeviceHost::Stop();
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnAction(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    /* parse the action name */
    NPT_String name = action->GetActionDesc()->GetName();

    // ContentDirectory
    if (name.Compare("Browse", true) == 0) {
        return OnBrowse(action, info);
    }
    if (name.Compare("Search", true) == 0) {
        return OnSearch(action, info);
    }
    if (name.Compare("GetSystemUpdateID", true) == 0) {
        return OnGetSystemUpdateID(action, info);
    }
    if (name.Compare("GetSortCapabilities", true) == 0) {
        return OnGetSortCapabilities(action, info);
    }  
    if (name.Compare("GetSearchCapabilities", true) == 0) {
        return OnGetSearchCapabilities(action, info);
    }  

    // ConnectionMananger
    if (name.Compare("GetCurrentConnectionIDs", true) == 0) {
        return OnGetCurrentConnectionIDs(action, info);
    }
    if (name.Compare("GetProtocolInfo", true) == 0) {
        return OnGetProtocolInfo(action, info);
    }    
    if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
        return OnGetCurrentConnectionInfo(action, info);
    }

    action->SetError(401,"No Such Action.");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionIDs
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetCurrentConnectionIDs(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("ConnectionIDs"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetProtocolInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetProtocolInfo(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Source"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Sink"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetCurrentConnectionInfo(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

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
|   PLT_MediaServer::OnGetSystemUpdateID
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSystemUpdateID(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("Id"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSortCapabilities
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSortCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentValue("SortCaps", "*"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnGetSearchCapabilities
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnGetSearchCapabilities(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    if (NPT_FAILED(action->SetArgumentValue("SearchCaps", "*"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
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
|   PLT_MediaServer::OnBrowse
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaServer::OnBrowse(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    NPT_Result    res;
    NPT_String    object_id;
    NPT_String    browseFlagValue;

    if (NPT_FAILED(action->GetArgumentValue("ObjectId", object_id)) || 
        NPT_FAILED(action->GetArgumentValue("BrowseFlag",  browseFlagValue))) {
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - invalid arguments.");
        return NPT_SUCCESS;
    }

    /* we don't support sorting yet */
//    PLT_Argument* sortCritArg = action->GetArgument("SortCriteria");
//     if (sortCritArg != NULL)  {
//         NPT_String sortCrit = sortCritArg->GetValue();
//         if (sortCrit.GetLength() > 0) {
//             /* error */
//             PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - SortCriteria not allowed.");
//             action->SetError(709, "MediaServer does not support sorting.");
//             return NPT_FAILURE;
//         }
//     }

    /* extract browseFlag */
    BrowseFlags browseFlag;
    if (NPT_FAILED(GetBrowseFlag(browseFlagValue, browseFlag))) {
        /* error */
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - BrowseFlag value not allowed.");
        action->SetError(402,"Invalid BrowseFlag arg.");
        return NPT_SUCCESS;
    }

    PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::On%s - id = %s\r\n", (const char*)browseFlagValue, (const char*)object_id);

    /* Invoke the browse function */
    if (browseFlag == BROWSEMETADATA) {
        res = OnBrowseMetadata(action, object_id, info);
    } else {
        res = OnBrowseDirectChildren(action, object_id, info);
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
PLT_MediaServer::OnSearch(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_COMPILER_UNUSED(info);

    NPT_Result    res;
    NPT_String    container_id;
    //PLT_Argument* searchCritArg = action->GetArgument("SearchCriteria");

    if (NPT_FAILED(action->GetArgumentValue("ContainerId", container_id))) {
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - invalid arguments.");
        return NPT_FAILURE;
    }

    /* we don't support sorting yet */
//    PLT_Argument* sortCritArg = action->GetArgument("SortCriteria");
//     if (sortCritArg != NULL)  {
//         NPT_String sortCrit = sortCritArg->GetValue();
//         if (sortCrit.GetLength() > 0) {
//             /* error */
//             PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - SortCriteria not allowed.");
//             action->SetError(709, "MediaServer does not support sorting.");
//             return NPT_FAILURE;
//         }
//     }

    PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnSearch - id = %s\r\n", (const char*)container_id);

    /* Invoke the browse function */
    res = OnBrowseDirectChildren(action, container_id);

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseMetadata(PLT_ActionReference& action, 
                                  const char* object_id, 
                                  NPT_SocketInfo* info /* = NULL */)
{ 
    NPT_COMPILER_UNUSED(action);
    NPT_COMPILER_UNUSED(object_id);
    NPT_COMPILER_UNUSED(info);

    return NPT_ERROR_NOT_IMPLEMENTED; 
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::OnBrowseDirectChildren(PLT_ActionReference& action, 
                                        const char*          object_id, 
                                        NPT_SocketInfo*      info /* = NULL */) 
{ 
    NPT_COMPILER_UNUSED(action);
    NPT_COMPILER_UNUSED(object_id);
    NPT_COMPILER_UNUSED(info);

    return NPT_ERROR_NOT_IMPLEMENTED; 
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::ProcessFileRequest
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaServer::ProcessFileRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response) 
{ 
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(info);
    NPT_COMPILER_UNUSED(response);

    return NPT_ERROR_NOT_IMPLEMENTED; 
}


