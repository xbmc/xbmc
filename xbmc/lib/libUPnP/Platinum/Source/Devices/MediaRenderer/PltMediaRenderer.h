/*****************************************************************
|
|   Platinum - AV Media Renderer Device
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
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
class PLT_MediaRendererInterface
{
public:
    virtual ~PLT_MediaRendererInterface() {}

    // ConnectionManager
    virtual NPT_Result OnGetCurrentConnectionInfo(PLT_ActionReference& action) = 0;

    // AVTransport
    virtual NPT_Result OnNext(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnPause(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnPlay(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnPrevious(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnSeek(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnStop(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnSetAVTransportURI(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnSetPlayMode(PLT_ActionReference& action) = 0;

    // RenderingControl
    //virtual NPT_Result OnGetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action) = 0;
    virtual NPT_Result OnSetMute(PLT_ActionReference& action) = 0;
};

/*----------------------------------------------------------------------
|   PLT_MediaRenderer class
+---------------------------------------------------------------------*/
class PLT_MediaRenderer : public PLT_DeviceHost,
                          public PLT_MediaRendererInterface
{
public:
    PLT_MediaRenderer(const char*          friendly_name,
                      bool                 show_ip = false,
                      const char*          uuid = NULL,
                      unsigned int         port = 0);

    // PLT_DeviceHost methods
    virtual NPT_Result OnAction(PLT_ActionReference&          action, 
                                const NPT_HttpRequestContext& context);

    // class methods
    static void SetupServices(PLT_DeviceData& data);

protected:
    virtual ~PLT_MediaRenderer();

    // PLT_MediaRendererInterface methods
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

    // RenderingControl
    //virtual NPT_Result OnGetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetVolume(PLT_ActionReference& action);
    virtual NPT_Result OnSetMute(PLT_ActionReference& action);
};

#endif /* _PLT_MEDIA_RENDERER_H_ */
