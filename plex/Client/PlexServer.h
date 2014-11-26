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

#include "utils/XBMCTinyXML.h"

class CPlexServer;
class CPlexConnection;
typedef boost::shared_ptr<CPlexServer> CPlexServerPtr;
typedef boost::shared_ptr<CPlexConnection> CPlexConnectionPtr;

#define PLEX_SERVER_CLASS_SECONDARY "secondary"

class CPlexServerConnTestThread : public CThread
{
  public:
    CPlexServerConnTestThread(CPlexConnectionPtr conn, CPlexServerPtr server);
    void Process();
    void Cancel();

    CPlexConnectionPtr m_conn;
    CPlexServerPtr m_server;
};

class CPlexServer : public boost::enable_shared_from_this<CPlexServer>
{
public:
  CPlexServer(const CStdString& uuid, const CStdString& name, bool owned, bool synced = false)
    : m_owned(owned), m_uuid(uuid), m_name(name), m_synced(synced), m_lastRefreshed(0), m_home(false) {}

  CPlexServer() {}

  CPlexServer(CPlexConnectionPtr connection);

  virtual ~CPlexServer();
  
  bool CollectDataFromRoot(const CStdString xmlData);
  CStdString toString() const;

  bool HasActiveLocalConnection() const;
  CPlexConnectionPtr GetLocalConnection() const;
  void MarkAsRefreshing();
  bool MarkUpdateFinished(int connType);

  void Merge(CPlexServerPtr otherServer);

  bool UpdateReachability();
  void CancelReachabilityTests();

  CStdString GetAccessToken() const;

  CStdString GetName() const { return m_name; }
  CStdString GetUUID() const { return m_uuid; }
  CStdString GetVersion() const { return m_version; }
  CStdString GetOwner() const { return m_owner; }
  CStdString GetServerClass() const { return m_serverClass; }
  bool GetOwned() const { return m_owned; }
  bool IsComplete() const { return m_complete; }
  bool GetSynced() const { return m_synced; }
  bool GetHome() const { return m_home; }
  bool IsShared() const { return !m_owned && !m_home; }

  CPlexServerPtr GetShared() { return shared_from_this(); }
  CPlexConnectionPtr GetActiveConnection() const;
  CURL GetActiveConnectionURL() const;

  bool Equals(const CPlexServerPtr& otherServer) { return m_uuid.Equals(otherServer->m_uuid); }

  /* ConnTestThread */
  void OnConnectionTest(CPlexServerConnTestThread *thread, CPlexConnectionPtr conn, int state);

  void GetConnections(std::vector<CPlexConnectionPtr> &conns);
  int GetNumConnections() const;

  CURL BuildURL(const CStdString& path, const CStdString& options="") const;
  CURL BuildPlexURL(const CStdString& path) const;
  void AddConnection(CPlexConnectionPtr connection);

  void SetUUID(const CStdString &uuid) { m_uuid = uuid; }
  void SetName(const CStdString &name) { m_name = name; }
  void SetOwned(bool owned) { m_owned = owned; }
  void SetHome(bool home) { m_home = home; }
  void SetOwner(const CStdString &owner) { m_owner = owner; }
  void SetSynced(bool synced) { m_synced = synced; }
  void SetVersion(const CStdString& version) { m_version = version; }
  void SetServerClass(const CStdString& classStr) { m_serverClass = classStr; }
  void SetSupportsVideoTranscoding(bool support) { m_supportsVideoTranscoding = support; }
  void SetSupportsAudioTranscoding(bool support) { m_supportsAudioTranscoding = support; }
  void SetSupportsDeletion(bool support) { m_supportsDeletion = support; }

  void SetTranscoderQualities(std::vector<std::string>& qualties) { m_transcoderQualities = qualties; }
  void SetTranscoderBitrates(std::vector<std::string>& bitrates) { m_transcoderQualities = bitrates; }
  void SetTranscoderResolutions(std::vector<std::string>& resolutions) { m_transcoderResolutions = resolutions; }
  
  std::vector<std::string> GetTranscoderQualities() const { return m_transcoderQualities; }
  std::vector<std::string> GetTranscoderBitrates() const { return m_transcoderBitrates; }
  std::vector<std::string> GetTranscoderResolutions() const { return m_transcoderResolutions; }
  
  bool SupportsVideoTranscoding() const { return m_supportsVideoTranscoding; }
  bool SupportsAudioTranscoding() const { return m_supportsAudioTranscoding; }
  bool SupportsDeletion() const { return m_supportsDeletion; }
  
  bool HasAuthToken() const;
  std::string GetAnyToken() const;
    
  void SetActiveConnection(CPlexConnectionPtr connection) { m_activeConnection = connection; }

  uint64_t GetLastRefreshed() const { return m_lastRefreshed; }
  void DidRefresh() { m_lastRefreshed = XbmcThreads::SystemClockMillis(); }

  bool IsSecondary() const
  {
    return (m_serverClass == "secondary");
  }

private:
  bool m_owned;
  bool m_home;
  bool m_synced;
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

  std::vector<CPlexConnectionPtr> m_connections;
  CPlexConnectionPtr m_activeConnection;
  CPlexConnectionPtr m_bestConnection;

  int m_connectionsLeft;
  bool m_complete;

  boost::timer m_connTestTimer;

  CCriticalSection m_serverLock;

  CCriticalSection m_testingLock;
  CEvent m_testEvent;
  CEvent m_noMoreConnThreads;

  CCriticalSection m_connTestThreadLock;
  std::vector<CPlexServerConnTestThread*> m_connTestThreads;

  uint64_t m_lastRefreshed;
};
