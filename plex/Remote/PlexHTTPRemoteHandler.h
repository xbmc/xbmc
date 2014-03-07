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

#include "utils/XBMCTinyXML.h"

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
    /* generic function to handle commandID parameter */
    void updateCommandID(const HTTPRequest &request, const ArgMap &arguments);

    /* find server for playMedia and showDetails commands */
    CPlexServerPtr getServerFromArguments(const ArgMap &arguments);

    /* player functions */
    void playMedia(const ArgMap &arguments);
    void stepFunction(const CStdString &url, const ArgMap &arguments);
    void skipNext(const ArgMap &arguments);
    void skipPrevious(const ArgMap &arguments);
    void pausePlay(const ArgMap &arguments);
    void stop(const ArgMap &arguments);
    void seekTo(const ArgMap &arguments);
    void showDetails(const ArgMap &arguments);
    void navigation(const CStdString &url, const ArgMap &arguments);
    void set(const ArgMap &arguments);
    void setVolume(const ArgMap &arguments);
    void sendString(const ArgMap &arguments);
    void sendVKey(const ArgMap &arguments);
    void subscribe(const HTTPRequest &request, const ArgMap &arguments);
    void unsubscribe(const HTTPRequest &request, const ArgMap &arguments);
    void setStreams(const ArgMap &arguments);
    void poll(const HTTPRequest &request, const ArgMap &arguments);
    void skipTo(const ArgMap &arguments);
    void resources();

    void setStandardResponse(int code=200, const CStdString status="OK")
    {
      m_xmlOutput.Clear();

      TiXmlDeclaration decl("1.0", "utf-8", "");
      m_xmlOutput.InsertEndChild(decl);

      TiXmlElement el("Response");
      el.SetAttribute("code", code);
      el.SetAttribute("status", std::string(status));
      m_xmlOutput.InsertEndChild(el);
    }

    CPlexRemoteSubscriberPtr getSubFromRequest(const HTTPRequest &request, const ArgMap &arguments);
    CXBMCTinyXML m_xmlOutput;

    CStdString m_data;
};


#endif /* defined(__Plex_Home_Theater__PlexHTTPRemoteHandler__) */
