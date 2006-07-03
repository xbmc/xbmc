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

class CUPnP
{
public:
    CUPnP();
    ~CUPnP();

    void Init();
    bool IsInitted() { return m_Initted; }

    PLT_UPnP* m_UPnP;
    PLT_CtrlPointReference m_CtrlPoint;
    PLT_SyncMediaBrowser*  m_MediaBrowser;

private:
    bool m_Initted;
};

extern CUPnP g_UPnP;

namespace DIRECTORY
{
class CUPnPDirectory :  public IDirectory
{
public:
    CUPnPDirectory(void);
    virtual ~CUPnPDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

private:
};
}
