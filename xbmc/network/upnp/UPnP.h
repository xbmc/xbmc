/*
 * UPnP Support for XBMC
 *      Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *      Copyright (C) 2006-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>

class NPT_LogHandler;
class PLT_UPnP;
class PLT_SyncMediaBrowser;
class PLT_MediaController;
class PLT_MediaObject;
class PLT_MediaItemResource;
class CFileItem;
class CBookmark;

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
    bool StartServer();
    void StopServer();

    // client
    void StartClient();
    void StopClient();
    bool IsClientStarted() { return (m_MediaBrowser != NULL); }

    // renderer
    bool StartRenderer();
    void StopRenderer();
    void UpdateState();

    // class methods
    static CUPnP* GetInstance();
    static CUPnPServer* GetServer();
    static void   ReleaseInstance(bool bWait);
    static bool   IsInstantiated() { return upnp != NULL; }

    static bool MarkWatched(const CFileItem& item,
                            const bool watched);

    static bool SaveFileState(const CFileItem& item,
                              const CBookmark& bookmark,
                              const bool updatePlayCount);

    static void RegisterUserdata(void* ptr);
    static void UnregisterUserdata(void* ptr);
private:
    // methods
    CUPnPRenderer* CreateRenderer(int port = 0);
    CUPnPServer*   CreateServer(int port = 0);

public:
    PLT_SyncMediaBrowser*       m_MediaBrowser;
    PLT_MediaController*        m_MediaController;

private:
    std::string                 m_IP;
    PLT_UPnP*                   m_UPnP;
    NPT_LogHandler*             m_LogHandler;
    CDeviceHostReferenceHolder* m_ServerHolder;
    CRendererReferenceHolder*   m_RendererHolder;
    CCtrlPointReferenceHolder*  m_CtrlPointHolder;


    static CUPnP* upnp;
};

} /* namespace UPNP */
