/*****************************************************************
|
|   Platinum - HTTP Server Listener
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

#ifndef _PLT_HTTP_SERVER_LISTENER_H_
#define _PLT_HTTP_SERVER_LISTENER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_HttpServerListener Interface
+---------------------------------------------------------------------*/
class PLT_HttpServerListener
{
 public:
    virtual ~PLT_HttpServerListener() {}
    
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest&              request, 
                                          const NPT_HttpRequestContext& context,
                                          NPT_HttpResponse*&            response,
                                          bool&                         headers_only) = 0;
};

#endif /* _PLT_HTTP_SERVER_LISTENER_H_ */
