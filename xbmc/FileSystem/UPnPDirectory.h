#pragma once
#include "idirectory.h"

#include "../lib/libUPnP/Platinum.h"
#include "../lib/libUPnP/PltMediaServer.h"
#include "../lib/libUPnP/PltMediaBrowser.h"
#include "../lib/libUPnP/PltSyncMediaBrowser.h"

//#include "Platinum.h"
//#include "PltMediaServer.h"
//#include "PltMediaBrowser.h"
//#include "PltSyncMediaBrowser.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
class CUPnPDirectory;

class CUPnP
{
public:
    CUPnP();
    ~CUPnP();

    // methods
    static CUPnP* GetInstance();
    static void   ReleaseInstance();
    static bool   IsInstantiated() { return upnp != NULL; }

private:
    friend class CUPnPDirectory;

    PLT_UPnP*              m_UPnP;
    PLT_CtrlPointReference m_CtrlPoint;
    PLT_SyncMediaBrowser*  m_MediaBrowser;

    static CUPnP* upnp;
};

class CUPnPDirectory :  public IDirectory
{
public:
    CUPnPDirectory(void) {}
    virtual ~CUPnPDirectory(void) {}

    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

    static const char* GetFriendlyName(const char* url);

private:
};
}
