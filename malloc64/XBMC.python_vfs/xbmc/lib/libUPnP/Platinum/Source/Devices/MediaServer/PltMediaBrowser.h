/*****************************************************************
|
|   Platinum - AV Media Browser (Media Server Control Point)
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_MEDIA_BROWSER_H_
#define _PLT_MEDIA_BROWSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltCtrlPoint.h"
#include "PltMediaItem.h"
#include "PltMediaBrowserListener.h"

/*----------------------------------------------------------------------
|   PLT_MediaBrowser class
+---------------------------------------------------------------------*/
class PLT_MediaBrowser : public PLT_CtrlPointListener
{
public:
    PLT_MediaBrowser(PLT_CtrlPointReference&   ctrl_point, 
                     PLT_MediaBrowserListener* listener);
    virtual ~PLT_MediaBrowser();

    // PLT_CtrlPointListener methods
    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device);
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device);
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_ActionReference& action, void* userdata);
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars);

    // ContentDirectory service
    NPT_Result Browse(
        PLT_DeviceDataReference& device, 
        const char*              object_id, 
        NPT_UInt32               start_index,
        NPT_UInt32               count,
        bool                     browse_metadata = false,
        const char*              filter = "*",
        const char*              sort_criteria = "",
        void*                    userdata = NULL);

    // accessor methods
    const NPT_Lock<PLT_DeviceDataReferenceList>& GetMediaServers() { 
        return m_MediaServers; 
    }

protected:
    // ContentDirectory service responses
    virtual NPT_Result OnBrowseResponse(
        NPT_Result               res, 
        PLT_DeviceDataReference& device, 
        PLT_ActionReference&     action, 
        void*                    userdata);

protected:
    PLT_CtrlPointReference                m_CtrlPoint;
    PLT_MediaBrowserListener*             m_Listener;
    NPT_Lock<PLT_DeviceDataReferenceList> m_MediaServers;
};

#endif /* _PLT_MEDIA_BROWSER_H_ */
