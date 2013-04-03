#pragma once

#include "StdString.h"
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/timer.hpp>

#include "threads/CriticalSection.h"
#include "Job.h"
#include "JobManager.h"
#include "URL.h"

class CPlexServer;
class CPlexConnection;
typedef boost::shared_ptr<CPlexServer> CPlexServerPtr;
typedef boost::shared_ptr<CPlexConnection> CPlexConnectionPtr;

#define PLEX_SERVER_CLASS_SECONDARY "secondary"

class CPlexServerConnTestJob : public CJob
{
public:
  CPlexServerConnTestJob(CPlexConnectionPtr conn, CPlexServerPtr server) : CJob()
  {
    m_conn = conn;
    m_server = server;
  }

  bool DoWork();

  CPlexConnectionPtr m_conn;
  CPlexServerPtr m_server;
};

class CPlexServer : public boost::enable_shared_from_this<CPlexServer>, public CJobQueue
{
public:
  CPlexServer(const CStdString& uuid, const CStdString& name, bool owned)
  : CJobQueue(false, 4, CJob::PRIORITY_NORMAL), m_owned(owned), m_uuid(uuid), m_name(name) {}

  CPlexServer() {}

  void CollectDataFromRoot(const CStdString xmlData);
  CStdString toString() const;

  bool HasActiveLocalConnection() const;
  void MarkAsRefreshing();
  bool MarkUpdateFinished(int connType);

  bool Merge(CPlexServerPtr otherServer);

  bool UpdateReachability();

  CStdString GetName() const { return m_name; }
  CStdString GetUUID() const { return m_uuid; }
  CStdString GetVersion() const { return m_version; }
  CStdString GetOwner() const { return m_owner; }
  CStdString GetServerClass() const { return m_serverClass; }
  bool GetOwned() const { return m_owned; }

  CPlexServerPtr GetShared() { return shared_from_this(); }
  CPlexConnectionPtr GetActiveConnection() const;
  CURL GetActiveConnectionURL() const;

  bool operator== (const CPlexServer& otherServer) { return m_uuid.Equals(otherServer.m_uuid); }

  /* CJobQueue members */
  virtual void OnJobComplete(unsigned int jobId, bool success, CJob *job);

  void GetConnections(std::vector<CPlexConnectionPtr> &conns);
  int GetNumConnections() const;

  CURL BuildURL(const CStdString& path) const;
  CURL BuildPlexURL(const CStdString& path) const;
  void AddConnection(CPlexConnectionPtr connection);

private:
  bool m_owned;
  CStdString m_uuid;
  CStdString m_name;
  CStdString m_version;
  CStdString m_owner;
  CStdString m_serverClass;

  bool m_supportsDeletion;
  bool m_supportsAudioTranscoding;
  bool m_supportsVideoTranscoding;

  std::vector<std::string> m_transcoderQualities;
  std::vector<std::string> m_transcoderBitrates;
  std::vector<std::string> m_transcoderResolutions;

  CCriticalSection m_connLock;
  std::vector<CPlexConnectionPtr> m_connections;
  CPlexConnectionPtr m_activeConnection;
  CPlexConnectionPtr m_bestConnection;

  int m_connectionsLeft;
  bool m_complete;

  boost::timer m_connTestTimer;

  CCriticalSection m_testingLock;
  CEvent m_testEvent;
};
