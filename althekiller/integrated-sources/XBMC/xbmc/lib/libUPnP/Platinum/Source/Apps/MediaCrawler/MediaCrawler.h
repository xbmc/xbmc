/*****************************************************************
|
|   Platinum - Media Crawler
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _CRAWLER_H_
#define _CRAWLER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Platinum.h"
#include "PltFileMediaServer.h"
#include "PltSyncMediaBrowser.h"
#include "StreamHandler.h"

/*----------------------------------------------------------------------
|   CMediaCrawler
+---------------------------------------------------------------------*/
class CMediaCrawler : public PLT_MediaBrowser,
                      public PLT_FileMediaServer

{
public:
    CMediaCrawler(PLT_CtrlPointReference& ctrlPoint,
                  const char*             friendly_name = "Platinum Crawler",
                  bool                    show_ip = false,
                  const char*             udn = NULL,
                  unsigned int            port = 0,
                  unsigned int            fileserver_port = 0);
    virtual ~CMediaCrawler();

    NPT_Result AddStreamHandler(CStreamHandler* handler);

    // accessor methods
    NPT_HttpUrl GetFileBaseUri() const { return m_FileBaseUri; }

protected:
    // PLT_MediaServer methods
    NPT_Result OnBrowse(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);

    // PLT_MediaBrowser methods
    NPT_Result OnBrowseResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata);
    
    // File Server Listener methods
    NPT_Result ProcessFileRequest(NPT_HttpRequest& request, NPT_HttpResponse& response, NPT_SocketInfo& info);

private:
    // methods
    NPT_Result OnBrowseRoot(PLT_ActionReference& action);
    NPT_Result OnBrowseDevice(PLT_ActionReference& action, 
                              const char*          server_uuid, 
                              const char*          server_object_id, 
                              NPT_SocketInfo*      info = NULL);

    NPT_Result SplitObjectId(const NPT_String& object_id, NPT_String& server_uuid, NPT_String& server_object_id);
    NPT_String FormatObjectId(const NPT_String& server_uuid, const NPT_String& server_object_id);

    NPT_String UpdateDidl(const char* server_uuid, const NPT_String& didl, NPT_SocketInfo* info = NULL);

    // members
    NPT_List<CStreamHandler*> m_StreamHandlers;
};

/*----------------------------------------------------------------------
|   CMediaCrawlerBrowseInfo
+---------------------------------------------------------------------*/
struct CMediaCrawlerBrowseInfo {
    NPT_SharedVariable shared_var;
    NPT_Result         res;
    int                code;
    NPT_String         object_id;
    NPT_String         didl;
    NPT_String         nr;
    NPT_String         tm;
    NPT_String         uid;
};

typedef NPT_Reference<CMediaCrawlerBrowseInfo> CMediaCrawlerBrowseInfoReference;

#endif /* _CRAWLER_H_ */

