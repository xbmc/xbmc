/*****************************************************************
|
|   Platinum - AV Media Renderer Device
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_MEDIA_RENDERER_H_
#define _PLT_MEDIA_RENDERER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDeviceHost.h"

/*----------------------------------------------------------------------
|   PLT_MediaRenderer class
+---------------------------------------------------------------------*/
class PlaybackCmdListener
{
public:
    NPT_Result OnNext();
    NPT_Result OnPause();
    NPT_Result OnPlay();
    NPT_Result OnPrevious();
    NPT_Result OnSeek();
    NPT_Result OnStop();
    NPT_Result OnOpen();
    NPT_Result OnSetPlayMode();
};

/*----------------------------------------------------------------------
|   PLT_MediaRenderer class
+---------------------------------------------------------------------*/
class PLT_MediaRenderer : public PLT_DeviceHost
{
public:
    PLT_MediaRenderer(PlaybackCmdListener* listener,
                      const char*          friendly_name,
                      bool                 show_ip = false,
                      const char*          uuid = NULL,
                      unsigned int         port = 0);

    // PLT_DeviceHost methods
    virtual NPT_Result OnAction(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

    void ReportPlaybackStatus();

protected:
    virtual ~PLT_MediaRenderer();

    // ConnectionManager
    virtual NPT_Result OnGetCurrentConnectionInfo(PLT_ActionReference& action);

    // AVTransport
    virtual NPT_Result OnNext(PLT_ActionReference& action);
    virtual NPT_Result OnPause(PLT_ActionReference& action);
    virtual NPT_Result OnPlay(PLT_ActionReference& action);
    virtual NPT_Result OnPrevious(PLT_ActionReference& action);
    virtual NPT_Result OnSeek(PLT_ActionReference& action);
    virtual NPT_Result OnStop(PLT_ActionReference& action);
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action);
    virtual NPT_Result OnSetPlayMode(PLT_ActionReference& action);
};

#endif /* _PLT_MEDIA_RENDERER_H_ */
