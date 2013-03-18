#pragma once

#include "URL.h"
#include "StdString.h"
#include "Client/PlexServer.h"
#include "PlexUtils.h"

class CPlexConnection
{
  enum ConnectionType
  {
    CONNECTION_MANUAL,
    CONNECTION_DISCOVERED,
    CONNECTION_MYPLEX
  };

  enum ConnectionState
  {
    CONNECTION_STATE_UNKNOWN,
    CONNECTION_STATE_UNREACHABLE,
    CONNECTION_STATE_REACHABLE,
    CONNECTION_STATE_UNAUTHORIZED
  };

  CPlexConnection() {}
  CPlexConnection(ConnectionType type, const CStdString& host, int port, const CStdString& token);

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

  bool operator== (const CPlexConnection &other);

public:
  ConnectionType m_type;
  ConnectionState m_state;

  CURL m_url;
  CStdString m_token;

  bool m_refreshed;

};