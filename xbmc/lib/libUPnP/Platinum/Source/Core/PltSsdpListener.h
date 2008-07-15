/*****************************************************************
|
|   Platinum - SSDP Listener
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_SSDP_LISTENER_H_
#define _PLT_SSDP_LISTENER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_SsdpPacketListener class
+---------------------------------------------------------------------*/
class PLT_SsdpPacketListener
{
public:
    virtual ~PLT_SsdpPacketListener() {}
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest& request, NPT_SocketInfo info) = 0;
};

/*----------------------------------------------------------------------
|   PLT_SsdpSearchResponseListener class
+---------------------------------------------------------------------*/
class PLT_SsdpSearchResponseListener
{
public:
    virtual ~PLT_SsdpSearchResponseListener() {}
    virtual NPT_Result ProcessSsdpSearchResponse(NPT_Result        res, 
                                                 NPT_SocketInfo&   info, 
                                                 NPT_HttpResponse* response) = 0;
};

#endif /* _PLT_SSDP_LISTENER_H_ */
