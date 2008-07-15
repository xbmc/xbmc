/*****************************************************************
|
|   Platinum - Stream Handler
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_STREAM_HANDLER_H_
#define _PLT_STREAM_HANDLER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptStreams.h"
#include "NptStrings.h"
#include "NptTime.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class CMediaCrawler;

/*----------------------------------------------------------------------
|   CStreamHandler
+---------------------------------------------------------------------*/
class CStreamHandler
{
public:
    CStreamHandler(CMediaCrawler* crawler) : m_MediaCrawler(crawler) {}
    virtual ~CStreamHandler() {}

    // overridables
    virtual bool       HandleResource(const char* prot_info, const char* url) = 0;
    virtual NPT_Result ModifyResource(NPT_XmlElementNode* resource, NPT_SocketInfo* info = NULL) = 0;
    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest& request, NPT_HttpResponse*& response) = 0;

protected:
    CMediaCrawler* m_MediaCrawler;
};

/*----------------------------------------------------------------------
|   CStreamHandlerFinder
+---------------------------------------------------------------------*/
class CStreamHandlerFinder
{
public:
    // methods
    CStreamHandlerFinder(const char* prot_info, const char* url) : m_ProtInfo(prot_info), m_Url(url) {}
    bool operator()(CStreamHandler* const & handler) const {
        return handler->HandleResource(m_ProtInfo, m_Url);
    }

private:
    // members
    NPT_String m_ProtInfo;
    NPT_String m_Url;
};

/*----------------------------------------------------------------------
|   CPassThroughStreamHandler
+---------------------------------------------------------------------*/
class CPassThroughStreamHandler : public CStreamHandler
{
public:
    CPassThroughStreamHandler(CMediaCrawler* crawler) : CStreamHandler(crawler) {}
    virtual ~CPassThroughStreamHandler() {}

    // overridables
    virtual bool HandleResource(const char* /*prot_info*/, const char* /*url*/) {
        return true;
    }

    virtual NPT_Result ModifyResource(NPT_XmlElementNode* resource, 
                                      NPT_SocketInfo*     info = NULL) {
        NPT_COMPILER_UNUSED(resource);
        NPT_COMPILER_UNUSED(info);
        return NPT_SUCCESS;
    }

    virtual NPT_Result ProcessFileRequest(NPT_HttpRequest&   request, 
                                          NPT_HttpResponse*& response) {
        NPT_HttpClient client;
        return client.SendRequest(request, response);
    }

};

#endif /* _PLT_STREAM_HANDLER_H_ */
