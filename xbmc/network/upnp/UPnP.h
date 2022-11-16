/*
 * UPnP Support for XBMC
 *  Copyright (c) 2006 c0diq (Sylvain Rebaud)
 *      Portions Copyright (c) by the authors of libPlatinum
 *      http://www.plutinosoft.com/blog/category/platinum/
 *  Copyright (C) 2006-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

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

    // controller
    void StartController();
    void StopController();
    bool IsControllerStarted() { return (m_MediaController != NULL); }

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
    static bool UpdateItem(const std::string& path,
                           const CFileItem& item);

    static void RegisterUserdata(void* ptr);
    static void UnregisterUserdata(void* ptr);
private:
    CUPnP(const CUPnP&) = delete;
    CUPnP& operator=(const CUPnP&) = delete;

    void CreateControlPoint();
    void DestroyControlPoint();

    // methods
    CUPnPRenderer* CreateRenderer(int port = 0);
    CUPnPServer*   CreateServer(int port = 0);

    CCriticalSection m_lockMediaBrowser;

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
