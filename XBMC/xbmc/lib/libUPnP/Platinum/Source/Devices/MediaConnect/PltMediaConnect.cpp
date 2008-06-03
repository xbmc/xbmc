/*****************************************************************
|
|      Platinum - AV Media Connect Device
|
|      (c) 2004 Sylvain Rebaud
|      Author: Sylvain Rebaud (c0diq@yahoo.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "Platinum.h"
#include "PltMediaConnect.h"
//#include "MACFromIP.h"

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
                                   unsigned int port,
                                   NPT_UInt16   fileserver_port) :	
    PLT_FileMediaServer(path, friendly_name, show_ip, udn, port, fileserver_port)
{
    m_RegistrarService = new PLT_Service(
        this,
        "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
        "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar");

    if (NPT_SUCCEEDED(m_RegistrarService->SetSCPDXML((const char*) X_MS_MediaReceiverRegistrarSCPD))) {
        m_RegistrarService->InitURLs("X_MS_MediaReceiverRegistrar", m_UUID);
        AddService(m_RegistrarService);
        m_RegistrarService->SetStateVariable("AuthorizationGrantedUpdateID", "0", false);
        m_RegistrarService->SetStateVariable("AuthorizationDeniedUpdateID", "0", false);
        m_RegistrarService->SetStateVariable("ValidationSucceededUpdateID", "0", false);
        m_RegistrarService->SetStateVariable("ValidationRevokedUpdateID", "0", false);
    }

    m_ModelName = "Windows Media Connect";
    m_ModelNumber = "2.0";
    m_Manufacturer = "Microsoft";
    m_ManufacturerURL = "http://www.microsoft.com/";
    m_ModelURL = "http://www.microsoft.com";
    m_DlnaDoc = "DMS-1.00";

    //NPT_String uuid;
    //GenerateUID(19, uuid);
    //m_SerialNumber = "{19C813F4-6C40-475A-9CFE-E33005B797F1}";
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::~PLT_MediaConnect
+---------------------------------------------------------------------*/
PLT_MediaConnect::~PLT_MediaConnect()
{
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
PLT_MediaConnect::OnAction(PLT_ActionReference& action, 
                           NPT_SocketInfo*      info /* = NULL */)
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

    return PLT_FileMediaServer::OnAction(action, info);
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
