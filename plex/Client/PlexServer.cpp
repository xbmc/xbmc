#include "Client/PlexServer.h"
#include "utils/XBMCTinyXML.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "PlexConnection.h"

#include <boost/foreach.hpp>
#include <boost/timer.hpp>

using namespace std;

bool
CPlexServerConnTestJob::DoWork()
{
  if (!m_conn->IsLocal())
    Sleep(50);

  CPlexConnection::ConnectionState state = m_conn->TestReachability(m_server);
  if (state == CPlexConnection::CONNECTION_STATE_REACHABLE)
  {
    CLog::Log(LOGDEBUG, "CPlexServerConnTestJob:DoWork Connection SUCCESS %s ~ localConn: %s conn: %s",
              m_server->GetName().c_str(), m_conn->IsLocal() ? "YES" : "NO", m_conn->GetAddress().Get().c_str());
    return true;
  }

  CLog::Log(LOGDEBUG, "CPlexServerConnTestJob:DoWork Connection FAILURE %s ~ localConn: %s conn: %s",
            m_server->GetName().c_str(), m_conn->IsLocal() ? "YES" : "NO", m_conn->GetAddress().Get().c_str());

  return false;
}

void
CPlexServer::CollectDataFromRoot(const CStdString xmlData)
{
  CXBMCTinyXML doc;
  doc.Parse(xmlData);
  if (doc.RootElement() != 0)
  {
    TiXmlElement* root = doc.RootElement();
    bool boolValue;

    if (root->QueryBoolAttribute("allowMediaDeletion", &boolValue))
      m_supportsDeletion = boolValue;

    if (root->QueryBoolAttribute("transcoderAudio", &boolValue))
      m_supportsAudioTranscoding = boolValue;

    if (root->QueryBoolAttribute("transcoderVideo", &boolValue))
      m_supportsVideoTranscoding = boolValue;

    root->QueryStringAttribute("serverClass", &m_serverClass);
    root->QueryStringAttribute("version", &m_version);

    CStdString stringValue;
    if (root->QueryStringAttribute("transcoderVideoResolutions", &stringValue))
      m_transcoderResolutions = StringUtils::Split(stringValue, ",");

    if (root->QueryStringAttribute("transcoderVideoBitrates", &stringValue))
      m_transcoderBitrates = StringUtils::Split(stringValue, ",");

    if (root->QueryStringAttribute("transcoderVideoQualities", &stringValue))
      m_transcoderQualities = StringUtils::Split(stringValue, ",");

    CLog::Log(LOGDEBUG, "CPlexServer::CollectDataFromRoot knowledge complete: %s", toString().c_str());
  }
}

bool
CPlexServer::HasActiveLocalConnection() const
{
  CSingleLock lk(m_connLock);

  return (m_activeConnection != NULL && m_activeConnection->IsLocal());
}

void
CPlexServer::MarkAsRefreshing()
{
  CSingleLock lk(m_connLock);
  BOOST_FOREACH(CPlexConnectionPtr conn, m_connections)
    conn->SetRefreshed(false);
}

bool
CPlexServer::MarkUpdateFinished(int connType)
{
  vector<CPlexConnectionPtr> connsToRemove;

  CSingleLock lk(m_connLock);
  BOOST_FOREACH(CPlexConnectionPtr conn, m_connections)
  {
    if (conn->GetRefreshed() == false)
      conn->m_type &= ~connType;

    if (conn->m_type == 0)
      connsToRemove.push_back(conn);
  }

  BOOST_FOREACH(CPlexConnectionPtr conn, connsToRemove)
  {
    CLog::Log(LOGDEBUG, "CPlexServer::MarkUpdateFinished Removing connection for %s after update finished for type %d: %s", m_name.c_str(), connType, conn->toString().c_str());
    vector<CPlexConnectionPtr>::iterator it = find(m_connections.begin(), m_connections.end(), conn);
    m_connections.erase(it);
  }

  return m_connections.size() > 0;
}

bool
ConnectionSortFunction(CPlexConnectionPtr c1, CPlexConnectionPtr c2)
{
  if (c1->IsLocal() && !c2->IsLocal()) return false;
  if (!c1->IsLocal() && c2->IsLocal()) return true;
  return c1->GetAddress().Get() < c2->GetAddress().Get();
}

bool
CPlexServer::UpdateReachability()
{
  CSingleLock lk(m_connLock);

  if (m_connections.size() == 0)
    return false;

  m_connTestTimer.restart();
  CLog::Log(LOGDEBUG, "CPlexServer::UpdateReachability Updating reachability for %s with %ld connections.", m_name.c_str(), m_connections.size());

  m_bestConnection.reset();
  m_connectionsLeft = m_connections.size();
  m_complete = false;

  vector<CPlexConnectionPtr> sortedConnections = m_connections;
  sort(sortedConnections.begin(), sortedConnections.end(), ConnectionSortFunction);

  BOOST_FOREACH(CPlexConnectionPtr conn, sortedConnections)
  {
    CLog::Log(LOGDEBUG, "CPlexServer::UpdateReachability testing connection %s", conn->GetAddress().Get().c_str());
    AddJob(new CPlexServerConnTestJob(conn, GetShared()));
  }

  if (m_testEvent.WaitMSec(30000))
  {
    CSingleLock lk(m_testingLock);
    m_complete = true;
    m_activeConnection = m_bestConnection;
  }

  CLog::Log(LOGDEBUG, "CPlexServer::OnJobComplete Connectivity test to %s completed in %.1f Seconds -> %s",
            m_name.c_str(), m_connTestTimer.elapsed(), m_activeConnection ? m_activeConnection->toString().c_str() : "FAILED");

  return (bool)m_bestConnection;
}

void
CPlexServer::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexServerConnTestJob* conTestJob = (CPlexServerConnTestJob*)job;

  CSingleLock lk(m_testingLock);
  if(success)
  {
    if(!m_bestConnection)
      m_bestConnection = conTestJob->m_conn;
    else if (conTestJob->m_conn->IsLocal() && !m_bestConnection->IsLocal())
      m_activeConnection = conTestJob->m_conn;
  }

  if (--m_connectionsLeft == 0 && m_complete == false)
    m_testEvent.Set();
}

void
CPlexServer::Merge(CPlexServerPtr otherServer)
{
  m_name = otherServer->m_name;
  m_version = otherServer->m_version;
  m_owned = otherServer->m_owned;
  m_owner = otherServer->m_owner;

  CSingleLock lk(m_connLock);
  BOOST_FOREACH(CPlexConnectionPtr conn, otherServer->m_connections)
  {
    vector<CPlexConnectionPtr>::iterator it;
    it = find(m_connections.begin(), m_connections.end(), conn);
    if (it != m_connections.end())
      (*it)->Merge(conn);
    else
      m_connections.push_back(*it);
  }
}

void
CPlexServer::GetConnections(std::vector<CPlexConnectionPtr> &conns)
{
  CSingleLock lk(m_connLock);
  conns = m_connections;
}

int
CPlexServer::GetNumConnections() const
{
  CSingleLock lk(m_connLock);
  return m_connections.size();
}

CPlexConnectionPtr
CPlexServer::GetActiveConnection() const
{
  CSingleLock lk(m_connLock);
  return m_activeConnection;
}

CURL
CPlexServer::GetActiveConnectionURL() const
{
  CSingleLock lk(m_connLock);
  return m_activeConnection->GetAddress();
}

CURL
CPlexServer::BuildPlexURL(const CStdString& path) const
{
  CURL url;
  url.SetProtocol("plex");
  url.SetHostName(m_uuid);
  url.SetFileName(path);
  return url.Get();
}

CURL
CPlexServer::BuildURL(const CStdString &path) const
{
  CSingleLock lk(m_connLock);
  CPlexConnectionPtr connection = m_activeConnection;

  if (!connection && m_connections.size() > 0)
    /* no active connection, just take the first one at random */
    connection = m_connections[0];
  else if (!connection && m_connections.size() == 0)
    /* no connections are no gooooood */
    return CURL();

  CURL url = connection->BuildURL(path);
  if (!url.HasOption("X-Plex-Token"))
  {
    /* See if we can find a token in our other connections */
    CStdString token;
    BOOST_FOREACH(CPlexConnectionPtr conn, m_connections)
    {
      if (!conn->GetAccessToken().empty())
      {
        token = conn->GetAccessToken();
        break;
      }
    }

    if (!token.empty())
      url.SetOption("X-Plex-Token", token);
  }
  return url;
}

void
CPlexServer::AddConnection(CPlexConnectionPtr connection)
{
  CSingleLock lk(m_connLock);
  m_connections.push_back(connection);
}

CStdString
CPlexServer::toString() const
{
  CStdString ret;
  ret.Format("%s version: %s owned: %s videoTranscode: %s audioTranscode: %s deletion: %s class: %s",
             m_name,
             m_version,
             m_owned ? "YES" : "NO",
             m_supportsVideoTranscoding ? "YES" : "NO",
             m_supportsAudioTranscoding ? "YES" : "NO",
             m_supportsDeletion ? "YES" : "NO",
             m_serverClass);

  return ret;
}