/*****************************************************************
|
|      Platinum - AV Media Connect Device
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
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "Platinum.h"
#include "PltMediaConnect.h"

NPT_SET_LOCAL_LOGGER("platinum.devices.mediaconnect")

/*----------------------------------------------------------------------
|       forward references
+---------------------------------------------------------------------*/
extern NPT_UInt8 X_MS_MediaReceiverRegistrarSCPD[];

/*----------------------------------------------------------------------
|       PLT_MediaConnect::PLT_MediaConnect
+---------------------------------------------------------------------*/
PLT_MediaConnect::PLT_MediaConnect(const char*  path, 
                                   const char*  friendly_name, 
                                   bool         show_ip     /* = false */, 
                                   const char*  uuid        /* = NULL */, 
                                   NPT_UInt16   port        /* = 0 */,
                                   bool         port_rebind /* = false */) :	
    PLT_FileMediaServer(path, friendly_name, show_ip, uuid, port, port_rebind),
    m_RegistrarService(NULL)
{
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::~PLT_MediaConnect
+---------------------------------------------------------------------*/
PLT_MediaConnect::~PLT_MediaConnect()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::SetupServices(PLT_DeviceData& data)
{
    m_RegistrarService = new PLT_Service(
        &data,
        "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
        "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar");

    NPT_CHECK_FATAL(m_RegistrarService->SetSCPDXML((const char*) X_MS_MediaReceiverRegistrarSCPD));
    NPT_CHECK_FATAL(m_RegistrarService->InitURLs("X_MS_MediaReceiverRegistrar", data.GetUUID()));
    NPT_CHECK_FATAL(data.AddService(m_RegistrarService));
    
    m_RegistrarService->SetStateVariable("AuthorizationGrantedUpdateID", "0");
    m_RegistrarService->SetStateVariable("AuthorizationDeniedUpdateID", "0");
    m_RegistrarService->SetStateVariable("ValidationSucceededUpdateID", "0");
    m_RegistrarService->SetStateVariable("ValidationRevokedUpdateID", "0");

    return PLT_FileMediaServer::SetupServices(data);
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::ProcessGetDescription
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaConnect::ProcessGetDescription(NPT_HttpRequest&              request,
                                        const NPT_HttpRequestContext& context,
                                        NPT_HttpResponse&             response)
{
    NPT_String m_OldModelName        = m_ModelName;
    NPT_String m_OldModelNumber      = m_ModelNumber;
    NPT_String m_OldModelURL         = m_ModelURL;
    NPT_String m_OldManufacturerURL  = m_ManufacturerURL;
    NPT_String m_OldDlnaDoc          = m_DlnaDoc;
    NPT_String m_OldDlnaCap          = m_DlnaCap;
    NPT_String m_OldAggregationFlags = m_AggregationFlags;

    // change some things based on User-Agent header
    NPT_HttpHeader* user_agent = request.GetHeaders().GetHeader(NPT_HTTP_HEADER_USER_AGENT);
    if (user_agent && user_agent->GetValue().Find("Xbox", 0, true)>=0) {
        m_ModelName        = "Windows Media Connect";
        m_ModelNumber      = "2.0";
        m_ModelURL         = "http://www.microsoft.com/";
        m_ManufacturerURL  = "http://www.microsoft.com/";
        m_DlnaDoc          = "";
        m_DlnaCap          = "";
        m_AggregationFlags = "";
        if (m_FriendlyName.Find(":") == -1)
            m_FriendlyName += ": 1";
        if (!m_FriendlyName.EndsWith(": Windows Media Connect")) 
            m_FriendlyName += ": Windows Media Connect";
    } else if (user_agent && user_agent->GetValue().Find("Sonos", 0, true)>=0) {
        m_ModelName        = "Windows Media Player Sharing";
        m_ModelNumber      = "3.0";
    }

    // PS3
    NPT_HttpHeader* client_info = request.GetHeaders().GetHeader("X-AV-Client-Info");
    if (client_info && client_info->GetValue().Find("PLAYSTATION 3", 0, true)>=0) {
        m_DlnaDoc = "DMS-1.50";
        m_DlnaCap = "av-upload,image-upload,audio-upload";
        m_AggregationFlags = "10";
    }

    NPT_Result res = PLT_FileMediaServer::ProcessGetDescription(request, context, response);
    
    // reset to old values now
    m_ModelName        = m_OldModelName;
    m_ModelNumber      = m_OldModelNumber;
    m_ModelURL         = m_OldModelURL;
    m_ManufacturerURL  = m_OldManufacturerURL;
    m_DlnaDoc          = m_OldDlnaDoc;
    m_DlnaCap          = m_OldDlnaCap;
    m_AggregationFlags = m_OldAggregationFlags;
    
    return res;
}


/*----------------------------------------------------------------------
|       PLT_MediaConnect::Authorize
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::Authorize(PLT_MediaConnectInfo* info, bool state)
{
    info->m_Authorized = state;
    if (state == true) {
        return m_RegistrarService->IncStateVariable("AuthorizationGrantedUpdateID");
    }

    return m_RegistrarService->IncStateVariable("AuthorizationDeniedUpdateID");
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::Validate
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::Validate(PLT_MediaConnectInfo* info, bool state)
{
    info->m_Validated = state;
    if (state == true) {
        return m_RegistrarService->IncStateVariable("ValidationSucceededUpdateID");
    }

    return m_RegistrarService->IncStateVariable("ValidationRevokedUpdateID");
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnAction(PLT_ActionReference&          action, 
                           const PLT_HttpRequestContext& context)
{
    PLT_MediaConnectInfo* mc_info = NULL;

    /* parse the action name */
    NPT_String name = action->GetActionDesc().GetName();

    /* handle X_MS_MediaReceiverRegistrar actions here */
    if (name.Compare("IsAuthorized") == 0) {
        return OnIsAuthorized(action, mc_info);
    }
    if (name.Compare("RegisterDevice") == 0) {
        return OnRegisterDevice(action, mc_info);
    }
    if (name.Compare("IsValidated") == 0) {
        return OnIsValidated(action, mc_info);
    }  

    return PLT_FileMediaServer::OnAction(action, context);
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::LookUpMediaConnectInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::LookUpMediaConnectInfo(NPT_String             deviceID, 
                                         PLT_MediaConnectInfo*& mc_info)
{
    mc_info = NULL;

    if (deviceID.GetLength()) {
        /* lookup the MAC from the UDN */
        NPT_String* MAC;
        if (NPT_SUCCEEDED(m_MediaConnectUDNMap.Get(deviceID, MAC))) {
            /* lookup the PLT_MediaConnectInfo from the MAC now */
            return m_MediaConnectDeviceInfoMap.Get(*MAC, mc_info);
        }
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnIsAuthorized
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnIsAuthorized(PLT_ActionReference&  action, 
                                 PLT_MediaConnectInfo* mc_info)
{
    bool authorized = true;

    NPT_String deviceID;
    action->GetArgumentValue("DeviceID", deviceID);

    /* is there a device ID passed ? */
    if (deviceID.GetLength()) {
        /* lookup the MediaConnectInfo from the UDN */
        NPT_String MAC;
        PLT_MediaConnectInfo* device_info;
        if (NPT_FAILED(LookUpMediaConnectInfo(deviceID, device_info))) {
            authorized = false;
        } else {
            authorized = device_info->m_Authorized;
        }
    } else {
        authorized = mc_info?mc_info->m_Authorized:true;
    }

    action->SetArgumentValue("Result", authorized?"1":"0");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnRegisterDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnRegisterDevice(PLT_ActionReference&  action, 
                                   PLT_MediaConnectInfo* mc_info)
{
    NPT_COMPILER_UNUSED(mc_info);

    NPT_String reqMsgBase64;
    action->GetArgumentValue("RegistrationReqMsg", reqMsgBase64);

    NPT_String respMsgBase64;
    action->SetArgumentValue("RegistrationRespMsg", respMsgBase64);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnIsValidated
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnIsValidated(PLT_ActionReference&  action, 
                                PLT_MediaConnectInfo* mc_info)
{
    bool validated = true;

    NPT_String deviceID;
    action->GetArgumentValue("DeviceID", deviceID);

    /* is there a device ID passed ? */
    if (deviceID.GetLength()) {
        /* lookup the MediaConnectInfo from the UDN */
        NPT_String MAC;
        PLT_MediaConnectInfo* device_info;
        if (NPT_FAILED(LookUpMediaConnectInfo(deviceID, device_info))) {
            validated = false;
        } else {
            validated = device_info->m_Validated;
        }
    } else {
        validated = mc_info?mc_info->m_Validated:true;
    }

    action->SetArgumentValue("Result", validated?"1":"0");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::GetFilePath
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::GetFilePath(const char* object_id, 
                              NPT_String& filepath) 
{
    if (!object_id) return NPT_ERROR_INVALID_PARAMETERS;

    // Reroute XBox360 and WMP requests to our route
    if (NPT_StringsEqual(object_id, "15")) {
        return PLT_FileMediaServer::GetFilePath("", filepath); // Videos
    } else if (NPT_StringsEqual(object_id, "16")) {
        return PLT_FileMediaServer::GetFilePath("", filepath); // Photos
    } else if (NPT_StringsEqual(object_id, "13")) {
        return PLT_FileMediaServer::GetFilePath("", filepath); // Music
    }

    return PLT_FileMediaServer::GetFilePath(object_id, filepath);;
}

