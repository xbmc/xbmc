//
//  PlexHTTPRemoteHandler.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-16.
//
//

#ifndef __Plex_Home_Theater__PlexHTTPRemoteHandler__
#define __Plex_Home_Theater__PlexHTTPRemoteHandler__

#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "utils/StdString.h"
#include "PlexRemoteSubscriberManager.h"

typedef std::map<std::string, std::string> ArgMap;

class CPlexHTTPRemoteHandler : public IHTTPRequestHandler
{
  public:
    CPlexHTTPRemoteHandler() {};
    virtual IHTTPRequestHandler* GetInstance() { return new CPlexHTTPRemoteHandler(); }
    virtual bool CheckHTTPRequest(const HTTPRequest &request);
    virtual int HandleHTTPRequest(const HTTPRequest &request);
  
    virtual void* GetHTTPResponseData() const;
    virtual size_t GetHTTPResonseDataLength() const;
  
  private:
    /* player functions */
    void playMedia(const ArgMap &arguments);
    void stepFunction(const CStdString &url, const ArgMap &arguments);
    void skipNext(const ArgMap &arguments);
    void skipPrevious(const ArgMap &arguments);
    void pausePlay(const ArgMap &arguments);
    void stop(const ArgMap &arguments);
    void seekTo(const ArgMap &arguments);
    void navigation(const CStdString &url, const ArgMap &arguments);
    void set(const ArgMap &arguments);
    void setVolume(const ArgMap &arguments);
    void sendString(const ArgMap &arguments);
    void sendVKey(const ArgMap &arguments);
    void subscribe(const HTTPRequest &request, const ArgMap &arguments);
    void unsubscribe(const HTTPRequest &request, const ArgMap &arguments);
    void setStreams(const ArgMap &arguments);
  
    CPlexRemoteSubscriberPtr getSubFromRequest(const HTTPRequest &request, const ArgMap &arguments);
    CStdString m_data;
};


#endif /* defined(__Plex_Home_Theater__PlexHTTPRemoteHandler__) */
