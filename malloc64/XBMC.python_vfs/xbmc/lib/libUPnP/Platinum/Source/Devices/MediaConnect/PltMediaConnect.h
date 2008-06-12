/*****************************************************************
|
|      Platinum - AV Media Connect Device
|
|      (c) 2004 Sylvain Rebaud
|      Author: Sylvain Rebaud (c0diq@yahoo.com)
|
 ****************************************************************/

#ifndef _PLT_MEDIA_CONNECT_H_
#define _PLT_MEDIA_CONNECT_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltFileMediaServer.h"

/*----------------------------------------------------------------------
|   PLT_MediaConnectInfo
+---------------------------------------------------------------------*/
class PLT_MediaConnectInfo
{
public:
    PLT_MediaConnectInfo(bool authorized = false) : 
        m_Authorized(authorized), m_Validated(false) {}

    bool       m_Authorized;
    bool       m_Validated;
    NPT_String m_UDN;
};

/*----------------------------------------------------------------------
|   defines
+---------------------------------------------------------------------*/
typedef NPT_Map<NPT_String, PLT_MediaConnectInfo>        PLT_MediaConnectDeviceInfoMap;
typedef NPT_Map<NPT_String, PLT_MediaConnectInfo>::Entry PLT_MediaConnectDeviceInfoMapEntry;
typedef NPT_Map<NPT_String, NPT_String>                  PLT_UDNtoMACMap;
typedef NPT_Map<NPT_String, NPT_String>::Entry           PLT_UDNtoMACMapEntry;

/*----------------------------------------------------------------------
|   PLT_MediaConnect
+---------------------------------------------------------------------*/
class PLT_MediaConnect : public PLT_FileMediaServer
{
public:
    PLT_MediaConnect(const char*  path, 
                     const char*  friendly_name,
                     bool         show_ip = false,
                     const char*  udn = NULL,
                     unsigned int port = 0,
                     NPT_UInt16   fileserver_port = 0);

    // PLT_DeviceHost methods
    NPT_Result OnAction(PLT_ActionReference& action, 
                        NPT_SocketInfo*      info = NULL);

protected:
    virtual ~PLT_MediaConnect();

    NPT_Result Authorize(PLT_MediaConnectInfo* info, bool state);
    NPT_Result Validate(PLT_MediaConnectInfo* info, bool state);

    // X_MS_MediaReceiverRegistrar
    virtual NPT_Result OnIsAuthorized(PLT_ActionReference&  action, 
                                      PLT_MediaConnectInfo* mediaConnectInfo);
    virtual NPT_Result OnRegisterDevice(PLT_ActionReference&  action, 
                                        PLT_MediaConnectInfo* mediaConnectInfo);
    virtual NPT_Result OnIsValidated(PLT_ActionReference&  action, 
                                     PLT_MediaConnectInfo* mediaConnectInfo);

private:
    NPT_Result LookUpMediaConnectInfo(NPT_String             deviceID, 
                                      PLT_MediaConnectInfo*& mediaConnectInfo);

protected:
    PLT_MediaConnectDeviceInfoMap m_MediaConnectDeviceInfoMap;
    PLT_UDNtoMACMap               m_MediaConnectUDNMap;

private:
    PLT_Service*                  m_RegistrarService;
};

#endif /* _PLT_MEDIA_CONNECT_H_ */
