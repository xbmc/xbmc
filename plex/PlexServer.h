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
  PlexServer(const string& uuid, const string& name, const string& addr, unsigned short port, const string& token)
    : uuid(uuid), name(name), address(addr), port(port), token(token), updatedAt(0), m_count(1)
  {
    // See if it's running on this machine.
    if (token.empty())
      local = Cocoa_IsHostLocal(addr);

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
    if (local) ret += 10;
    if (detected()) ret += 10;

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

  bool live;
  bool local;
  string uuid;
  string name;
  string token;
  string address;
  unsigned short port;
  time_t updatedAt;

 private:

  string m_key;
  int    m_count;
};


#endif // PLEXSERVER_H
