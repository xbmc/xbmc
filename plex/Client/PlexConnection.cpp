#include "PlexConnection.h"

#include "filesystem/CurlFile.h"

#include <boost/algorithm/string.hpp>

using namespace XFILE;

CPlexConnection::CPlexConnection(int type, const CStdString& host, int port, const CStdString& token) :
  m_type(type), m_state(CONNECTION_STATE_UNKNOWN), m_token(token)
{
  m_url.SetHostName(host);
  m_url.SetPort(port);
  m_refreshed = true;
  if (port == 443)
    m_url.SetProtocol("https");
  else
    m_url.SetProtocol("http");

  m_http.SetTimeout(3);
}

CURL
CPlexConnection::BuildURL(const CStdString &path) const
{
  CURL ret(m_url);
  CStdString p(path);

  if (boost::starts_with(path, "/"))
    p = path.substr(1, std::string::npos);

  ret.SetFileName(p);

  if (!GetAccessToken().empty())
    ret.SetOption(GetAccessTokenParameter(), GetAccessToken());

  return ret;
}

CPlexConnection::ConnectionState
CPlexConnection::TestReachability(CPlexServerPtr server)
{
  CURL url = BuildURL("/");
  CStdString rootXml;

  m_http.SetRequestHeader("Accept", "application/xml");

  if (GetAccessToken().empty() && server->HasAuthToken())
    url.SetOption(GetAccessTokenParameter(), server->GetAnyToken());

  if (m_http.Get(url.Get(), rootXml))
  {
    if (server->CollectDataFromRoot(rootXml))
      m_state = CONNECTION_STATE_REACHABLE;
    else
      /* if collect data from root fails, it can be because
       * we got a parser error from root XML == not good.
       * or we got a server we didn't expect == not good.
       * so let's just mark this connection as bad */
      m_state = CONNECTION_STATE_UNREACHABLE;
  }
  else
  {
    if (m_http.GetLastHTTPResponseCode() == 401)
      m_state = CONNECTION_STATE_UNAUTHORIZED;
    else
      m_state = CONNECTION_STATE_UNREACHABLE;
  }

  return m_state;
}

void
CPlexConnection::Merge(CPlexConnectionPtr otherConnection)
{
  m_url = otherConnection->m_url;
  m_token = otherConnection->m_token;
  m_type |= otherConnection->m_type;

  m_refreshed = true;
}

bool CPlexConnection::operator ==(const CPlexConnection &other)
{
  bool uriMatches = m_url.Get().Equals(other.GetAddress().Get());
  bool tokenMatches = GetAccessToken().Equals(other.GetAccessToken());

  return (uriMatches && tokenMatches);
}

CStdString
CPlexConnection::ConnectionStateName(CPlexConnection::ConnectionState state)
{
  switch (state) {
    case CONNECTION_STATE_REACHABLE:
      return "reachable";
      break;
    case CONNECTION_STATE_UNAUTHORIZED:
      return "unauthorized";
      break;
    case CONNECTION_STATE_UNKNOWN:
      return "unknown";
      break;
    default:
      return "unreachable";
      break;
  }
}

CStdString
CPlexConnection::ConnectionTypeName(CPlexConnection::ConnectionType type)
{
  CStdString typeName;
  if (type & CONNECTION_DISCOVERED)
    typeName = "(discovered)";
  if (type & CONNECTION_MANUAL)
    typeName += "(manual)";
  if (type & CONNECTION_MYPLEX)
    typeName += "(myplex)";

  return typeName;
}

void CPlexConnection::save(TiXmlNode* server)
{
  TiXmlElement conn("connection");

  conn.SetAttribute("host", m_url.GetHostName().c_str());
  conn.SetAttribute("port", m_url.GetPort());
  conn.SetAttribute("token", m_token.c_str());
  conn.SetAttribute("type", m_type);

  server->InsertEndChild(conn);
}

CPlexConnectionPtr CPlexConnection::load(TiXmlElement *element)
{
  int port, type;
  std::string host, token;

  if (element->QueryStringAttribute("host", &host) != TIXML_SUCCESS)
    return CPlexConnectionPtr();

  if (element->QueryStringAttribute("token", &token) != TIXML_SUCCESS)
    return CPlexConnectionPtr();

  if (element->QueryIntAttribute("port", &port) != TIXML_SUCCESS)
    return CPlexConnectionPtr();

  if (element->QueryIntAttribute("type", &type) != TIXML_SUCCESS)
    return CPlexConnectionPtr();

  CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(type, host, port, token));

  return connection;
}
