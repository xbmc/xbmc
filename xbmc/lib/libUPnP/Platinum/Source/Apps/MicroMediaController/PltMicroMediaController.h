/*****************************************************************
|
|   Platinum - Micro Media Controller
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _MICRO_MEDIA_CONTROLLER_H_
#define _MICRO_MEDIA_CONTROLLER_H_

#include "Platinum.h"
#include "PltMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "PltMediaController.h"
#include "NptMap.h"
#include "NptStack.h"

/*----------------------------------------------------------------------
 |   definitions
 +---------------------------------------------------------------------*/
typedef NPT_Map<NPT_String, NPT_String>        PLT_StringMap;
typedef NPT_Lock<PLT_StringMap>                PLT_LockStringMap;
typedef NPT_Map<NPT_String, NPT_String>::Entry PLT_StringMapEntry;

/*----------------------------------------------------------------------
 |   PLT_MediaItemIDFinder
 +---------------------------------------------------------------------*/
class PLT_MediaItemIDFinder
{
public:
    // methods
    PLT_MediaItemIDFinder(const char* object_id) : m_ObjectID(object_id) {}

    bool operator()(const PLT_MediaObject* const & item) const {
        return item->m_ObjectID.Compare(m_ObjectID, true) ? false : true;
    }

private:
    // members
    NPT_String m_ObjectID;
};

/*----------------------------------------------------------------------
 |   PLT_MicroMediaController
 +---------------------------------------------------------------------*/
class PLT_MicroMediaController : public PLT_MediaControllerListener
{
public:
    PLT_MicroMediaController(PLT_CtrlPointReference& ctrlPoint);
    virtual ~PLT_MicroMediaController();

    void ProcessCommandLoop();

    // PLT_MediaControllerListener
    void OnMRAddedRemoved(PLT_DeviceDataReference& device, int added);
    void OnMRStateVariablesChanged(PLT_Service* /* service */, NPT_List<PLT_StateVariable*>* /* vars */) {};

private:
    const char* ChooseIDFromTable(PLT_StringMap& table);
    void        PopDirectoryStackToRoot(void);
    NPT_Result  DoBrowse();

    void        GetCurMediaServer(PLT_DeviceDataReference& server);
    void        GetCurMediaRenderer(PLT_DeviceDataReference& renderer);

    PLT_DeviceDataReference ChooseDevice(const NPT_Lock<PLT_DeviceMap>& deviceList);

    // Command Handlers
    void    HandleCmd_scan(const char* ip);
    void    HandleCmd_getms();
    void    HandleCmd_setms();
    void    HandleCmd_ls();
    void    HandleCmd_cd();
    void    HandleCmd_cdup();
    void    HandleCmd_pwd();
    void    HandleCmd_help();
    void    HandleCmd_getmr();
    void    HandleCmd_setmr();
    void    HandleCmd_open();
    void    HandleCmd_play();
    void    HandleCmd_stop();

private:
    /* The tables of known devices on the network.  These are updated via the
     * OnMSAddedRemoved and OnMRAddedRemoved callbacks.  Note that you should first lock
     * before accessing them using the NPT_Map::Lock function.
     */
    NPT_Lock<PLT_DeviceMap> m_MediaServers;
    NPT_Lock<PLT_DeviceMap> m_MediaRenderers;

    /* The UPnP MediaServer control point. */
    PLT_SyncMediaBrowser*   m_MediaBrowser;

    /* The UPnP MediaRenderer control point. */
    PLT_MediaController*    m_MediaController;

    /* The currently selected media server as well as 
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaServer;
    NPT_Mutex               m_CurMediaServerLock;

    /* The currently selected media renderer as well as 
     * a lock.  If you ever want to hold both the m_CurMediaRendererLock lock and the 
     * m_CurMediaServerLock lock, make sure you grab the server lock first.
     */
    PLT_DeviceDataReference m_CurMediaRenderer;
    NPT_Mutex               m_CurMediaRendererLock;

    /* the most recent results from a browse request.  The results come back in a 
     * callback instead of being returned to the calling function, so this 
     * global variable is necessary in order to give the results back to the calling 
     * function.
     */
    PLT_MediaObjectListReference m_MostRecentBrowseResults;

    /* When browsing through the tree on a media server, this is the stack 
     * symbolizing the current position in the tree.  The contents of the 
     * stack are the object ID's of the nodes.  Note that the object id: "0" should
     * always be at the bottom of the stack.
     */
    NPT_Stack<NPT_String> m_CurBrowseDirectoryStack;

    /* the semaphore on which to block when waiting for a response from over
     * the network 
     */
    NPT_SharedVariable m_CallbackResponseSemaphore;
};

#endif /* _MICRO_MEDIA_CONTROLLER_H_ */

