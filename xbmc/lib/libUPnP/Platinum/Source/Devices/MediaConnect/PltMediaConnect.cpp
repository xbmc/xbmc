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
                                   bool         show_ip, 
                                   const char*  udn, 
                                   unsigned int port) :	
    PLT_FileMediaServer(path, friendly_name, show_ip, udn, port),
    m_RegistrarService(NULL)
{
    m_ModelName        = "Windows Media Player Sharing"; // for Xbox3630 & Sonos to accept us
    m_ModelNumber      = "3.0";                        // must be >= 3.0 for Sonos to accept us
    m_ModelDescription = "Media Server";
    m_Manufacturer     = "Plutinosoft";
    m_ManufacturerURL  = "http://www.plutinosoft.com/";
    m_ModelURL         = "http://www.plutinosoft.com";
    m_DlnaDoc          = "DMS-1.00";
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
    NPT_String m_OldModelName   = m_ModelName;
    NPT_String m_OldModelNumber = m_ModelNumber;

    // change some things based on User-Agent header
    NPT_HttpHeader* user_agent = request.GetHeaders().GetHeader(NPT_HTTP_HEADER_USER_AGENT);
    if (user_agent && user_agent->GetValue().Find("Xbox", 0, true)>=0) {
        // For the XBox 360 to discover us, ModelName must stay "Windows Media Player Sharing"
        m_ModelName        = "Windows Media Player Sharing";
        m_ModelNumber      = "3.0";

        // return modified description
        NPT_String doc;
        NPT_Result res = GetDescription(doc);

        // reset to old values now
        m_ModelName   = m_OldModelName;
        m_ModelNumber = m_OldModelNumber;

        NPT_CHECK_FATAL(res);

        PLT_HttpHelper::SetBody(response, doc);    
        PLT_HttpHelper::SetContentType(response, "text/xml");

        return NPT_SUCCESS;
    }

    return PLT_FileMediaServer::ProcessGetDescription(request, context, response);
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
                           const NPT_HttpRequestContext& context)
{
      PLT_MediaConnectInfo* mc_info = NULL;

//    /* get MAC address from IP */
//    if (info != NULL) {
//        NPT_String ip = info->remote_address.GetIpAddress().ToString();
//        NPT_String MAC;
//        GetMACFromIP(ip, MAC);
//
//        if (MAC.GetLength()) {
//            NPT_Result res = m_MediaConnectDeviceInfoMap.Get(MAC, mc_info);
//            if (NPT_FAILED(res)) {
//                m_MediaConnectDeviceInfoMap.Put(MAC, PLT_MediaConnectInfo());
//                m_MediaConnectDeviceInfoMap.Get(MAC, mc_info);
//
//                // automatically validate for now
//                Authorize(mc_info, true);
//            }
//        }
//    }
//
//    /* verify device is allowed first */
//    if (mc_info == NULL || !mc_info->m_Authorized) {
//        action->SetError(801, "Access Denied");
//        return NPT_SUCCESS;
//    }

    /* parse the action name */
    NPT_String name = action->GetActionDesc()->GetName();

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
