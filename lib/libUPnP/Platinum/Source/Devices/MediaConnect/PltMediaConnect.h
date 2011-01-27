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
                     NPT_UInt16   port = 0,
                     bool         port_rebind = false);

    NPT_Result Authorize(PLT_MediaConnectInfo* info, bool state);
    NPT_Result Validate(PLT_MediaConnectInfo* info, bool state);

protected:
    virtual ~PLT_MediaConnect();


    // PLT_DeviceHost methods
    virtual NPT_Result SetupServices(PLT_DeviceData& data);
    virtual NPT_Result OnAction(PLT_ActionReference&          action, 
                                const PLT_HttpRequestContext& context);
    virtual NPT_Result ProcessGetDescription(NPT_HttpRequest&              request,
                                             const NPT_HttpRequestContext& context,
                                             NPT_HttpResponse&             response);

    // PLT_FileMediaServer methods
    virtual NPT_Result GetFilePath(const char* object_id, NPT_String& filepath);

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
