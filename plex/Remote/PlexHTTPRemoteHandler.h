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
#include "PlexUtils.h"

#include "utils/XBMCTinyXML.h"

typedef std::map<std::string, std::string> ArgMap;

class CPlexRemoteResponse
{
public:
  CPlexRemoteResponse(int _code = 200, const std::string& status = "OK")
  {
    code = _code;

    CXBMCTinyXML xmlOutput;
    TiXmlDeclaration decl("1.0", "utf-8", "");
    xmlOutput.InsertEndChild(decl);

    TiXmlElement el("Response");
    el.SetAttribute("code", code);
    el.SetAttribute("status", std::string(status));
    xmlOutput.InsertEndChild(el);

    body = PlexUtils::GetXMLString(xmlOutput);
  }

  CPlexRemoteResponse(const CXBMCTinyXML& xmlResponse)
  {
    body = PlexUtils::GetXMLString(xmlResponse);
  }

  CPlexRemoteResponse(const CPlexRemoteResponse& response)
  {
    code = response.code;
    body = response.body;
  }

  int code;
  CStdString body;
};

class IPlexRemoteHandler
{
public:
  virtual CPlexRemoteResponse handle(const CStdString& url, const ArgMap& arguments) = 0;
};

class CPlexHTTPRemoteHandler : public IHTTPRequestHandler
{
public:
  CPlexHTTPRemoteHandler();
  virtual IHTTPRequestHandler* GetInstance()
  {
    return new CPlexHTTPRemoteHandler();
  }

  virtual bool CheckHTTPRequest(const HTTPRequest& request);
  virtual int HandleHTTPRequest(const HTTPRequest& request);

  virtual void* GetHTTPResponseData() const;
  virtual size_t GetHTTPResonseDataLength() const;

  static CPlexServerPtr getServerFromArguments(const ArgMap& arguments);

private:
  /* generic function to handle commandID parameter */
  CPlexRemoteResponse updateCommandID(const HTTPRequest& request, const ArgMap& arguments);

  /* find server for playMedia and showDetails commands */
  CPlexRemoteSubscriberPtr getSubFromRequest(const HTTPRequest& request, const ArgMap& arguments);

  /* player functions */
  CPlexRemoteResponse subscribe(const HTTPRequest& request, const ArgMap& arguments);
  CPlexRemoteResponse unsubscribe(const HTTPRequest& request, const ArgMap& arguments);
  CPlexRemoteResponse poll(const HTTPRequest& request, const ArgMap& arguments);
  CPlexRemoteResponse resources();
  CPlexRemoteResponse showDetails(const ArgMap &arguments);

  CStdString m_data;
  int m_formerWindow;
};

#endif /* defined(__Plex_Home_Theater__PlexHTTPRemoteHandler__) */
