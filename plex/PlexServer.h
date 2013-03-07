//
//  PlexServer.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-01-14.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXSERVER_H
#define PLEXSERVER_H

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <filesystem/CurlFile.h>
#include <string>
#include "XBMCTinyXML.h"
#include "plex/Network/NetworkInterface.h"

using namespace std;
using namespace XFILE;

class PlexServer;
typedef boost::shared_ptr<PlexServer> PlexServerPtr;
typedef std::pair<std::string, PlexServerPtr> key_server_pair;

#define PMS_LIVE_SCORE 50

////////////////////////////////////////////////////////////////////
class PlexServer
{
 public:

  /// Constructor.
  PlexServer(const string& uuid, const string& name, const string& addr, unsigned short port, const string& token, const string& deviceClass="desktop")
    : uuid(uuid), name(name), address(addr), port(port), token(token), updatedAt(0), m_count(1), deviceClass(deviceClass), m_canDoWebkit(false)
  {
    local = NetworkInterface::IsLocalAddress(addr);

    // Default to live if we detected it.
    live = detected();

    // Compute the key for the server.
    m_key = uuid + "-" + address + "-" + boost::lexical_cast<string>(port);
  }

  /// Is it alive? Blocks, can take time.
  bool reachable()
  {
    CCurlFile  http;

    /* set short timeout */
    http.SetTimeout(1);

    CStdString resp;
    live = http.Get(url(), resp);

    if (live)
    {
      CXBMCTinyXML doc;
      doc.Parse(resp);
      if (doc.RootElement() != 0)
      {
        TiXmlElement* el = doc.RootElement();
        if (el->Attribute("webkit"))
        {
          CStdString webkit(el->Attribute("webkit"));
          if (webkit == "1")
            m_canDoWebkit = true;
        }
        
        if (el->Attribute("transcoderVideoQualities"))
        {
          m_canTranscode = true;
        }
      }
    }

    return live;
  }

  /// Return the root URL.
  string url()
  {
    string ret = (port == 443 ? "https://" : "http://") + address + ":" + boost::lexical_cast<string>(port);
    if (token.empty() == false)
      ret += "?X-Plex-Token=" + token;

    return ret;
  }

  /// The score for the server.
  int score()
  {
    int ret = 0;

    // Bonus for being alive, being localhost, and being detected.
    if (live) ret += PMS_LIVE_SCORE;
    if (local) ret += 20;
    if (detected()) ret += 20;

    /* non mobile classes get a big bonus */
    if (!isMobile()) ret += 30;

    if (canDoWebkit()) ret += 10;
    if (canTranscode()) ret += 10;

    return ret;
  }

  /// The server key for hashing.
  string key() const
  {
    return m_key;
  }

  /// Was it auto-detected?
  bool detected()
  {
    return token.empty();
  }

  /// Equality operator.
  bool equals(const PlexServerPtr& rhs)
  {
    if (!rhs)
      return false;

    return (uuid == rhs->uuid && address == rhs->address && port == rhs->port);
  }

  /// Ref counting.
  void incRef()
  {
    ++m_count;
  }

  int decRef()
  {
    return --m_count;
  }

  int refCount() const
  {
    return m_count;
  }

  bool isMobile() const
  {
    return deviceClass == "mobile";
  }

  bool canDoWebkit() const
  {
    return m_canDoWebkit;
  }

  bool canTranscode() const
  {
    return m_canTranscode;
  }

  bool live;
  bool local;
  string uuid;
  string name;
  string token;
  string address;
  string deviceClass;
  unsigned short port;
  time_t updatedAt;
  bool m_canDoWebkit;
  bool m_canTranscode;

 private:

  string m_key;
  int    m_count;
};


#endif // PLEXSERVER_H
