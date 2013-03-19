#pragma once

#include "URL.h"
#include "StdString.h"
#include "PlexUtils.h"

#include "Client/PlexServer.h"

#include <boost/shared_ptr.hpp>

class CPlexConnection;
typedef boost::shared_ptr<CPlexConnection> CPlexConnectionPtr;

class CPlexConnection
{
public:
  enum ConnectionType
  {
    CONNECTION_MANUAL = 0x01,
    CONNECTION_DISCOVERED = 0x02,
    CONNECTION_MYPLEX = 0x04
  };

  enum ConnectionState
  {
    CONNECTION_STATE_UNKNOWN,
    CONNECTION_STATE_UNREACHABLE,
    CONNECTION_STATE_REACHABLE,
    CONNECTION_STATE_UNAUTHORIZED
  };

  CPlexConnection() {}
  CPlexConnection(int type, const CStdString& host, int port, const CStdString& token);

  static CStdString ConnectionTypeName(ConnectionType type);
  static CStdString ConnectionStateName(ConnectionState state);

  ConnectionState TestReachability(CPlexServerPtr server);
  CURL BuildURL(const CStdString& path);

  bool IsLocal() const
  {
    return PlexUtils::IsLocalNetworkIP(m_url.GetHostName());
  }

  CURL GetAddress() const
  {
    return m_url;
  }

  CStdString GetAccessToken() const
  {
    return m_token;
  }

  CStdString GetAccessTokenParameter() const
  {
    return "X-Plex-Token";
  }

  CStdString toString() const
  {
    CStdString fmt;
    fmt.Format("Connection: %s token used: %s type: %02x state: %s",
               m_url.Get().c_str(),
               m_token.empty() ? "NO" : "YES",
               m_type,
               ConnectionStateName(m_state).c_str());
    return fmt;
  }

  void Merge(CPlexConnectionPtr otherConnection);

  /* Setters, getters - Is this java? */
  void SetRefreshed(bool r) { m_refreshed = r; }
  bool GetRefreshed() const { return m_refreshed; }

  bool operator== (const CPlexConnection &other);

  int m_type;

private:
  ConnectionState m_state;

  CURL m_url;
  CStdString m_token;

  bool m_refreshed;

};