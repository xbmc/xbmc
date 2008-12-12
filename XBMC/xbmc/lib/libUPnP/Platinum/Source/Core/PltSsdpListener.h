/*****************************************************************
|
|   Platinum - SSDP Listener
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
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
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest&              request, 
                                    const NPT_HttpRequestContext& context) = 0;
};

/*----------------------------------------------------------------------
|   PLT_SsdpSearchResponseListener class
+---------------------------------------------------------------------*/
class PLT_SsdpSearchResponseListener
{
public:
    virtual ~PLT_SsdpSearchResponseListener() {}
    virtual NPT_Result ProcessSsdpSearchResponse(NPT_Result                    res,  
                                                 const NPT_HttpRequestContext& context,
                                                 NPT_HttpResponse*             response) = 0;
};

#endif /* _PLT_SSDP_LISTENER_H_ */
