/*
* UPnP Support for XBMC
* Copyright (c) 2006 c0diq (Sylvain Rebaud)
* Portions Copyright (c) by the authors of libPlatinum
*
* http://www.plutinosoft.com/blog/category/platinum/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#pragma once

#include "utils/StdString.h"

class PLT_UPnP;
class PLT_SyncMediaBrowser;
class PLT_MediaObject;
class PLT_MediaItemResource;

namespace UPNP
{

class CDeviceHostReferenceHolder;
class CCtrlPointReferenceHolder;
class CRendererReferenceHolder;
class CUPnPRenderer;
class CUPnPServer;

class CUPnP
{
public:
    CUPnP();
    ~CUPnP();

    // server
    void StartServer();
    void StopServer();

    // client
    void StartClient();
    void StopClient();
    bool IsClientStarted() { return (m_MediaBrowser != NULL); }

    // renderer
    void StartRenderer();
    void StopRenderer();
    void UpdateState();

    // class methods
    static CUPnP* GetInstance();
    static void   ReleaseInstance(bool bWait);
    static bool   IsInstantiated() { return upnp != NULL; }

private:
    // methods
    CUPnPRenderer* CreateRenderer(int port = 0);
    CUPnPServer*   CreateServer(int port = 0);

public:
    PLT_SyncMediaBrowser*       m_MediaBrowser;

private:
    CStdString                  m_IP;
    PLT_UPnP*                   m_UPnP;
    CDeviceHostReferenceHolder* m_ServerHolder;
    CRendererReferenceHolder*   m_RendererHolder;
    CCtrlPointReferenceHolder*  m_CtrlPointHolder;


    static CUPnP* upnp;
};

} /* namespace UPNP */
