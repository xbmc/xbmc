/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// FileNFS.cpp: implementation of the CNFSFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NFSFile.h"

#include "ServiceBroker.h"
#include "jobs/JobManager.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <inttypes.h>
#include <mutex>
#include <random>

#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs.h>

#ifdef TARGET_WINDOWS
#include <fcntl.h>
#include <sys\stat.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFDIR) != 0)
#endif
#endif

#if defined(TARGET_WINDOWS)
#define S_IRGRP 0
#define S_IROTH 0
#define S_IWUSR _S_IWRITE
#define S_IRUSR _S_IREAD
#endif

using namespace XFILE;

using namespace std::chrono_literals;

namespace
{
// Default "lease_time" on most Linux NFSv4 servers are 90s.
// See: https://linux-nfs.org/wiki/index.php/NFS_lock_recovery_notes
// Keep alive interval should be always less than lease_time to avoid client session expires

constexpr auto CONTEXT_TIMEOUT = 60s; // 2/3 parts of lease_time
constexpr auto KEEP_ALIVE_TIMEOUT = 45s; // half of lease_time
// close fast unused contexts when no active connections, and an export's extra contexts
constexpr auto IDLE_TIMEOUT = 30s;
constexpr int KEEP_ALIVE_READ_TIMEOUT_MS = 1000;

constexpr auto SETTING_NFS_VERSION = "nfs.version";
constexpr int DEFAULT_NFS_VERSION = 3; // libnfs's, when none is set
constexpr auto SETTING_NFS_CHUNKSIZE = "nfs.chunksize";

// libnfs names a context's NFSv4 client after the process ID and the second it was created in.
// The server treats a second context with the same name as that client rebooting and discards
// the first one's state, so give every context a name of its own.
std::string NextClientName()
{
  static const uint64_t processId = []
  {
    std::random_device random;
    return (static_cast<uint64_t>(random()) << 32) | random();
  }();
  static std::atomic<unsigned int> counter{0};
  return StringUtils::Format("Kodi {:016x} {}", processId, ++counter);
}

// rpc_init_context() seeds each context's xids from an unguarded static. Kodi's own contexts are
// created under this lock. Those made inside mount_getexports() and nfs_find_local_servers()
// still race with them, which is harmless: a context only matches replies against its own xids.
struct nfs_context* InitNfsContext()
{
  static std::mutex lock;
  std::unique_lock initLock(lock);
  return nfs_init_context();
}

// Whether a call's result means the server has dropped the context's NFSv4 client, as when its
// lease expired. libnfs up to 6.0.2 looks NFSv4 errors up in its NFSv3 table, so NFS4ERR_EXPIRED
// and the other NFSv4-only errors come back as -ERANGE, and NFS4ERR_DELAY as -EAGAIN; later
// versions return NFS4ERR_EXPIRED as -EAGAIN. Any other error in those groups costs one retry.
bool ClientDropped(const CNfsContext& context, int ret)
{
  return context.GetNfsVersion() == 4 && (ret == -ERANGE || ret == -EAGAIN);
}

// Serialises NFSv4 creates. libnfs makes their verifiers from an unguarded static, and two creates
// of one path that get the same verifier both succeed.
std::mutex nfs4CreateLock;

void ToStat64(const nfs_stat_64& nfsStat, struct __stat64* buffer)
{
  if (!buffer)
    return;

  *buffer = {};
  buffer->st_dev = nfsStat.nfs_dev;
  buffer->st_ino = nfsStat.nfs_ino;
  buffer->st_mode = nfsStat.nfs_mode;
  buffer->st_nlink = nfsStat.nfs_nlink;
  buffer->st_uid = nfsStat.nfs_uid;
  buffer->st_gid = nfsStat.nfs_gid;
  buffer->st_rdev = nfsStat.nfs_rdev;
  buffer->st_size = nfsStat.nfs_size;
  buffer->st_atime = nfsStat.nfs_atime;
  buffer->st_mtime = nfsStat.nfs_mtime;
  buffer->st_ctime = nfsStat.nfs_ctime;
}
} // unnamed namespace

class CNfsConnection::CMaintenanceJob : public CJob
{
public:
  CMaintenanceJob(CNfsConnection& connection,
                  std::vector<std::shared_ptr<CNfsContext>> keepAlives,
                  std::vector<std::shared_ptr<CNfsContext>> released)
    : m_connection(connection),
      m_keepAlives(std::move(keepAlives)),
      m_released(std::move(released))
  {
  }

  // also runs when the job is cancelled before it starts
  ~CMaintenanceJob() override { m_connection.m_maintenancePending = false; }

  bool DoWork() override
  {
    for (const auto& context : m_keepAlives)
    {
      // A busy context has a call in flight, which keeps its lease. Waiting for it would hold up
      // the keep alives of every other context, so it is left due for the next pass.
      std::unique_lock lock(*context, std::try_to_lock);
      if (!lock.owns_lock())
        continue;

      // handles closed since CheckIfIdle() are no longer in the list
      for (struct nfsfh* pFileHandle : context->GetDueKeepAlives(std::chrono::steady_clock::now()))
      {
        keepAlive(*context, pFileHandle);
        context->resetKeepAlive(pFileHandle);
      }
    }
    m_released.clear();
    return true;
  }

  const char* GetType() const override { return "nfsmaintenance"; }

private:
  CNfsConnection& m_connection;
  std::vector<std::shared_ptr<CNfsContext>> m_keepAlives;
  std::vector<std::shared_ptr<CNfsContext>> m_released;
};

CNfsConnection::CNfsConnection() : m_IdleTimeout(std::chrono::steady_clock::now() + IDLE_TIMEOUT)
{
}

CNfsConnection::~CNfsConnection()
{
  Deinit();
}

std::string CNfsConnection::resolveHost(const CURL& url)
{
  std::string resolvedHostName;
  CServiceBroker::GetDNSNameCache()->Lookup(url.GetHostName(), resolvedHostName);
  return resolvedHostName;
}

std::list<std::string> CNfsConnection::GetExportList(const CURL& url)
{
  std::list<std::string> retList;
  const std::string resolvedHostName = resolveHost(url);

  struct exportnode *exportlist, *tmp;
#ifdef HAS_NFS_MOUNT_GETEXPORTS_TIMEOUT
  exportlist = mount_getexports_timeout(
      resolvedHostName.c_str(),
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_nfsTimeout * 1000);
#else
  exportlist = mount_getexports(resolvedHostName.c_str());
#endif

  for (tmp = exportlist; tmp != NULL; tmp = tmp->ex_next)
  {
    std::string exportStr = std::string(tmp->ex_dir);

    retList.push_back(exportStr);
  }

  mount_free_export_list(exportlist);
  retList.sort();
  retList.reverse();

  return retList;
}

bool CNfsConnection::splitUrlIntoExportAndPath(const CURL& url,
                                               std::string& exportPath,
                                               std::string& relativePath)
{
  const std::string hostName = StringUtils::ToLower(url.GetHostName());
  std::list<std::string> exportList;
  {
    std::unique_lock lock(m_lock);
    exportList = m_exportLists[hostName];
  }

  //refresh exportlist if empty
  if (exportList.empty())
  {
    const auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return false;

    const auto settings = settingsComponent->GetSettings();
    if (!settings)
      return false;

    const int nfsVersion = settings->GetInt(SETTING_NFS_VERSION);

    if (nfsVersion == 4)
      exportList = {"/"};
    else
      exportList = GetExportList(url);

    std::unique_lock lock(m_lock);
    m_exportLists[hostName] = exportList;
  }

  return splitUrlIntoExportAndPath(url, exportPath, relativePath, exportList);
}

bool CNfsConnection::splitUrlIntoExportAndPath(const CURL& url,std::string &exportPath, std::string &relativePath, std::list<std::string> &exportList)
{
    bool ret = false;

    if(!exportList.empty())
    {
      relativePath = "";
      exportPath = "";

      std::string path = url.GetFileName();

      //GetFileName returns path without leading "/"
      //but we need it because the export paths start with "/"
      //and path.Find(*it) wouldn't work else
      if(path[0] != '/')
      {
        path = "/" + path;
      }

      for (const std::string& it : exportList)
      {
        //if path starts with the current export path
        if (URIUtils::PathHasParent(path, it))
        {
          /* It's possible that PathHasParent() may not find the correct match first/
           * As an example, if /path/ & and /path/sub/ are exported, but
           * the user specifies the path /path/subdir/ (from /path/ export).
           * If the path is longer than the exportpath, make sure / is next.
           */
          if ((path.length() > it.length()) && (path[it.length()] != '/') && it != "/")
            continue;
          exportPath = it;
          //handle special case where root is exported
          //in that case we don't want to strip off to
          //much from the path
          if( exportPath == path )
            relativePath = "//";
          else if( exportPath == "/" )
            relativePath = "//" + path.substr(exportPath.length());
          else
            relativePath = "//" + path.substr(exportPath.length()+1);
          ret = true;
          break;
        }
      }
    }
    return ret;
}

CNfsContext::CNfsContext(std::string key,
                         std::string resolvedHostName,
                         std::string exportPath,
                         struct nfs_context* pNfsContext)
  : m_key(std::move(key)),
    m_resolvedHostName(std::move(resolvedHostName)),
    m_exportPath(std::move(exportPath)),
    m_pNfsContext(pNfsContext),
    m_lastUsed(std::chrono::steady_clock::now())
{
}

CNfsContext::~CNfsContext()
{
  if (m_pNfsContext)
    nfs_destroy_context(m_pNfsContext);
}

void CNfsContext::Touch(std::chrono::time_point<std::chrono::steady_clock> now)
{
  std::unique_lock lock(m_stateLock);
  m_lastUsed = now;
}

bool CNfsContext::IsExpired(std::chrono::time_point<std::chrono::steady_clock> now,
                            std::chrono::steady_clock::duration timeout) const
{
  std::unique_lock lock(m_stateLock);
  return now - m_lastUsed > timeout;
}

CNfsContextLease::CNfsContextLease(CNfsContextPool& pool, std::shared_ptr<CNfsContext> context)
  : m_pool(&pool),
    m_context(std::move(context))
{
}

CNfsContextLease::~CNfsContextLease()
{
  Reset();
}

CNfsContextLease::CNfsContextLease(CNfsContextLease&& other) noexcept
  : m_pool(other.m_pool),
    m_context(std::move(other.m_context))
{
}

CNfsContextLease& CNfsContextLease::operator=(CNfsContextLease&& other) noexcept
{
  if (this != &other)
  {
    Reset();
    m_pool = other.m_pool;
    m_context = std::move(other.m_context);
  }
  return *this;
}

void CNfsContextLease::Reset()
{
  if (m_context)
  {
    m_pool->Release(*m_context);
    m_context.reset();
  }
}

CNfsContextLease CNfsContextPool::Checkout(const std::string& key,
                                           unsigned int maxContexts,
                                           const Factory& create)
{
  std::vector<std::shared_ptr<CNfsContext>> expired; // destroyed after m_lock is released
  std::unique_lock lock(m_lock);

  const auto now = std::chrono::steady_clock::now();
  auto& contexts = m_contexts[key];
  for (auto it = contexts.begin(); it != contexts.end();)
  {
    if ((*it)->m_users == 0 && (*it)->IsExpired(now, CONTEXT_TIMEOUT))
    {
      CLog::Log(LOGDEBUG, "NFS: Old context timed out - destroying it");
      expired.push_back(std::move(*it));
      it = contexts.erase(it);
    }
    else
      ++it;
  }

  std::shared_ptr<CNfsContext> context;
  const auto leastUsed = std::ranges::min_element(contexts, {}, [](const auto& candidate)
                                                  { return candidate->m_users; });
  if (leastUsed != contexts.end() && ((*leastUsed)->m_users == 0 || contexts.size() >= maxContexts))
  {
    context = *leastUsed;
  }
  else
  {
    context = create();
    if (!context)
    {
      if (contexts.empty())
        m_contexts.erase(key);
      return {};
    }
    contexts.push_back(context);
  }

  context->Touch(now);
  context->m_users++;
  return CNfsContextLease(*this, std::move(context));
}

void CNfsContextPool::Release(CNfsContext& context)
{
  std::shared_ptr<CNfsContext> retired; // destroyed after m_lock is released
  std::unique_lock lock(m_lock);

  if (--context.m_users > 0 || !context.m_retired)
    return;

  const auto it = std::ranges::find_if(m_retired, [&context](const auto& retiredContext)
                                       { return retiredContext.get() == &context; });
  if (it != m_retired.end())
  {
    retired = std::move(*it);
    m_retired.erase(it);
  }
}

void CNfsContextPool::Retire(const CNfsContext& context)
{
  std::shared_ptr<CNfsContext> unused; // destroyed after m_lock is released
  std::unique_lock lock(m_lock);

  const auto entry = m_contexts.find(context.m_key);
  if (entry == m_contexts.end())
    return;

  auto& contexts = entry->second;
  const auto it = std::ranges::find_if(contexts, [&context](const auto& candidate)
                                       { return candidate.get() == &context; });
  if (it == contexts.end())
    return; // already retired

  if (context.m_users > 0)
  {
    (*it)->m_retired = true;
    m_retired.push_back(std::move(*it));
  }
  else
  {
    unused = std::move(*it);
  }
  contexts.erase(it);
  if (contexts.empty())
    m_contexts.erase(entry);
}

void CNfsContextPool::RetireAll()
{
  decltype(m_contexts) contexts; // destroyed after m_lock
  std::unique_lock lock(m_lock);
  contexts.swap(m_contexts);

  for (auto& entry : contexts)
  {
    for (auto& context : entry.second)
    {
      if (context->m_users > 0)
      {
        context->m_retired = true;
        m_retired.push_back(std::move(context));
      }
    }
  }
}

std::vector<std::shared_ptr<CNfsContext>> CNfsContextPool::TakeUnused(TimePoint now, bool all)
{
  std::vector<std::shared_ptr<CNfsContext>> unused;
  std::unique_lock lock(m_lock);

  for (auto entry = m_contexts.begin(); entry != m_contexts.end();)
  {
    auto& contexts = entry->second;
    for (auto it = contexts.begin(); it != contexts.end();)
    {
      const auto timeout = it == contexts.begin() ? CONTEXT_TIMEOUT : IDLE_TIMEOUT;
      if ((*it)->m_users == 0 && (all || (*it)->IsExpired(now, timeout)))
      {
        unused.push_back(std::move(*it));
        it = contexts.erase(it);
      }
      else
        ++it;
    }

    if (contexts.empty())
      entry = m_contexts.erase(entry);
    else
      ++entry;
  }
  return unused;
}

std::vector<std::shared_ptr<CNfsContext>> CNfsContextPool::GetKeepAlivesDue(TimePoint now) const
{
  std::vector<std::shared_ptr<CNfsContext>> due;
  std::unique_lock lock(m_lock);

  for (const auto& entry : m_contexts)
  {
    for (const auto& context : entry.second)
    {
      if (!context->GetDueKeepAlives(now).empty())
        due.push_back(context);
    }
  }
  for (const auto& context : m_retired)
  {
    if (!context->GetDueKeepAlives(now).empty())
      due.push_back(context);
  }
  return due;
}

std::vector<struct nfsfh*> CNfsContext::GetDueKeepAlives(
    std::chrono::time_point<std::chrono::steady_clock> now) const
{
  std::vector<struct nfsfh*> handles;
  std::unique_lock lock(m_stateLock);
  for (const auto& it : m_keepAliveTimeouts)
  {
    if (it.second < now)
      handles.push_back(it.first);
  }
  return handles;
}

//reset timeouts on read
void CNfsContext::resetKeepAlive(struct nfsfh* pFileHandle)
{
  std::unique_lock lock(m_stateLock);
  //refresh last access time of the context aswell
  m_lastUsed = std::chrono::steady_clock::now();
  //adds new keys - refreshes existing ones
  m_keepAliveTimeouts[pFileHandle] = m_lastUsed + KEEP_ALIVE_TIMEOUT;
}

//remove file handle from keep alive list on file close
void CNfsContext::removeFromKeepAliveList(struct nfsfh* pFileHandle)
{
  std::unique_lock lock(m_stateLock);
  m_keepAliveTimeouts.erase(pFileHandle);
}

CNfsContextLease CNfsConnection::GetContext(const CURL& url, std::string& relativePath)
{
  std::string exportPath;
  if (!splitUrlIntoExportAndPath(url, exportPath, relativePath))
    return {};

  const std::string resolvedHostName = resolveHost(url);
  const std::string key = url.GetHostName() + exportPath;

  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  const auto advancedSettings =
      settingsComponent ? settingsComponent->GetAdvancedSettings() : nullptr;
  const unsigned int maxContexts = advancedSettings ? advancedSettings->m_nfsMaxConnections : 1;

  CNfsContextLease context = m_pool.Checkout(
      key, maxContexts,
      [&]() -> std::shared_ptr<CNfsContext>
      {
        CLog::Log(LOGDEBUG, "NFS: Creating a new context for {}.", key);
        struct nfs_context* pNfsContext = InitNfsContext();
        if (!pNfsContext)
        {
          CLog::Log(LOGERROR, "NFS: Error initcontext in GetContext.");
          return {};
        }
        return std::make_shared<CNfsContext>(key, resolvedHostName, exportPath, pNfsContext);
      });
  if (!context)
    return {};

  // Whoever locks a new context first mounts it, later callers wait here for the result.
  std::unique_lock lock(*context);
  if (context->m_mountState == CNfsContext::MountState::NONE)
    mount(*context, url.GetHostName());

  if (context->m_mountState != CNfsContext::MountState::MOUNTED)
    return {};

  return context;
}

// called with the context lock held
void CNfsConnection::mount(CNfsContext& context, const std::string& hostName)
{
  struct nfs_context* pNfsContext = context.GetNfsContext();
  context.m_nfsVersion = setOptions(pNfsContext);

  //we connect to the directory of the path. This will be the "root" path of this connection then.
  //So all fileoperations are relative to this mountpoint...
  const int nfsRet =
      nfs_mount(pNfsContext, context.m_resolvedHostName.c_str(), context.m_exportPath.c_str());

  if (nfsRet != 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to mount nfs share: {} ({})", context.m_exportPath,
              nfs_get_error(pNfsContext));
    context.m_mountState = CNfsContext::MountState::FAILED;
    RetireContext(context);
    return;
  }
  CLog::Log(LOGDEBUG, "NFS: Connected to server {} and export {}", hostName, context.m_exportPath);

  // read chunksize only works after mount
  uint64_t readChunkSize = nfs_get_readmax(pNfsContext);
  uint64_t writeChunkSize = nfs_get_writemax(pNfsContext);

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const uint64_t chunkSize =
      settings ? (settings->GetInt(SETTING_NFS_CHUNKSIZE) * 1024) : (128 * 1024);

  if (readChunkSize == 0)
  {
    CLog::Log(LOGDEBUG, "NFS Server did not return max read chunksize - Using setting value {}",
              chunkSize);
    readChunkSize = chunkSize;
  }
  else if (chunkSize < readChunkSize)
  {
    CLog::Log(LOGDEBUG,
              "NFS Server max read chunksize ({}) is bigger than client setting - Using client "
              "value {}",
              readChunkSize, chunkSize);
    readChunkSize = chunkSize;
  }

  if (writeChunkSize == 0)
  {
    CLog::Log(LOGDEBUG, "NFS Server did not return max write chunksize - Using setting value {}",
              chunkSize);
    writeChunkSize = chunkSize;
  }
  else if (chunkSize < writeChunkSize)
  {
    CLog::Log(LOGDEBUG,
              "NFS Server max write chunksize ({}) is bigger than client setting - Using client "
              "value {}",
              writeChunkSize, chunkSize);
    writeChunkSize = chunkSize;
  }

  CLog::Log(LOGDEBUG, "NFS: chunks: r/w {}/{}", (int)readChunkSize, (int)writeChunkSize);

  context.m_readChunkSize = readChunkSize;
  context.m_writeChunkSize = writeChunkSize;
  context.m_mountState = CNfsContext::MountState::MOUNTED;
}

void CNfsConnection::RetireContext(const CNfsContext& context)
{
  m_pool.Retire(context);
}

void CNfsConnection::Deinit()
{
  m_pool.RetireAll();

  std::unique_lock lock(m_lock);
  m_exportLists.clear();
}

/* This is called from CApplication::ProcessSlow() and is used to tell if nfs have been idle for too long */
void CNfsConnection::CheckIfIdle()
{
  // The keep alives and the release of contexts run as a job, as a keep alive waits on the
  // context lock and the server.
  const auto jobManager = CServiceBroker::GetJobManager();
  if (!jobManager || m_maintenancePending)
    return;

  const auto now = std::chrono::steady_clock::now();
  bool idle = false;
  {
    std::unique_lock lock(m_lock);
    idle = m_OpenConnections == 0 && m_IdleTimeout < now;
    // read again on the next use, so that a new export or a change of NFS version is picked up
    if (idle)
      m_exportLists.clear();
  }

  std::vector<std::shared_ptr<CNfsContext>> released = m_pool.TakeUnused(now, idle);
  std::vector<std::shared_ptr<CNfsContext>> keepAlives = m_pool.GetKeepAlivesDue(now);
  if (released.empty() && keepAlives.empty())
    return;

  if (idle && !released.empty())
    CLog::Log(LOGINFO, "NFS is idle. Closing the remaining connections.");

  // dedicated: the lower priorities share a few workers, which can stay busy past a keep alive's
  // deadline
  m_maintenancePending = true;
  jobManager->AddJob(new CMaintenanceJob(*this, std::move(keepAlives), std::move(released)),
                     nullptr, CJob::PRIORITY_DEDICATED);
}

//keep alive the filehandles nfs connection
//by blindly doing a read 32bytes - seek back to where
//we were before
// called with the context lock held
void CNfsConnection::keepAlive(CNfsContext& context, struct nfsfh* _pFileHandle)
{
  uint64_t offset = 0;
  char buffer[32];
  struct nfs_context* pContext = context.GetNfsContext();

  CLog::LogF(LOGDEBUG, "sending keep alive after {}s.",
             std::chrono::duration_cast<std::chrono::seconds>(KEEP_ALIVE_TIMEOUT).count());

  nfs_lseek(pContext, _pFileHandle, 0, SEEK_CUR, &offset);

  // Bounded, as it holds the context: closing the file, to stop playback or exit, waits for it.
  const int timeout = nfs_get_timeout(pContext);
  nfs_set_timeout(pContext, timeout > 0 ? std::min(timeout, KEEP_ALIVE_READ_TIMEOUT_MS)
                                        : KEEP_ALIVE_READ_TIMEOUT_MS);
#ifdef LIBNFS_API_V2
  int bytes = nfs_read(pContext, _pFileHandle, buffer, sizeof(buffer));
#else
  int bytes = nfs_read(pContext, _pFileHandle, sizeof(buffer), buffer);
#endif
  nfs_set_timeout(pContext, timeout);
  if (bytes < 0)
  {
    CLog::LogF(LOGERROR, "nfs_read - Error ({}, {})", bytes, nfs_get_error(pContext));
    return;
  }

  nfs_lseek(pContext, _pFileHandle, offset, SEEK_SET, &offset);
}

int CNfsConnection::stat(const CURL& url, nfs_stat_64* statbuff)
{
  int nfsRet = 0;
  std::string exportPath;
  std::string relativePath;
  struct nfs_context *pTmpContext = NULL;

  const std::string resolvedHostName = resolveHost(url);

  if(splitUrlIntoExportAndPath(url, exportPath, relativePath))
  {
    pTmpContext = InitNfsContext();

    if(pTmpContext)
    {
      setOptions(pTmpContext);
      //we connect to the directory of the path. This will be the "root" path of this connection then.
      //So all fileoperations are relative to this mountpoint...
      nfsRet = nfs_mount(pTmpContext, resolvedHostName.c_str(), exportPath.c_str());

      if(nfsRet == 0)
      {
        nfsRet = nfs_stat64(pTmpContext, relativePath.c_str(), statbuff);
      }
      else
      {
        CLog::Log(LOGERROR, "NFS: Failed to mount nfs share: {} ({})", exportPath,
                  nfs_get_error(pTmpContext));
      }

      nfs_destroy_context(pTmpContext);
      CLog::Log(LOGDEBUG, "NFS: Connected to server {} and export {} in tmpContext",
                url.GetHostName(), exportPath);
    }
  }
  return nfsRet;
}

/* The following two function is used to keep track on how many Opened files/directories there are.
needed for unloading the dylib*/
void CNfsConnection::AddActiveConnection()
{
  std::unique_lock lock(m_lock);
  m_OpenConnections++;
}

void CNfsConnection::AddIdleConnection()
{
  std::unique_lock lock(m_lock);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any weird behaviours if a user
   leaves the movie paused for a long while and then press stop */
  const auto now = std::chrono::steady_clock::now();
  m_IdleTimeout = now + IDLE_TIMEOUT;
}

int CNfsConnection::setOptions(struct nfs_context* context)
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return DEFAULT_NFS_VERSION;

  const auto advancedSettings = settingsComponent->GetAdvancedSettings();
  if (!advancedSettings)
    return DEFAULT_NFS_VERSION;

  uint32_t timeout = advancedSettings->m_nfsTimeout;
  nfs_set_timeout(context, timeout > 0 ? timeout * 1000 : -1);

  int retries = advancedSettings->m_nfsRetries;
  nfs_set_autoreconnect(context, retries);

  const auto settings = settingsComponent->GetSettings();
  if (!settings)
    return DEFAULT_NFS_VERSION;

  const int nfsVersion = settings->GetInt(SETTING_NFS_VERSION);

  int ret = nfs_set_version(context, nfsVersion);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to set nfs version: {} ({})", nfsVersion,
              nfs_get_error(context));
    return DEFAULT_NFS_VERSION;
  }

  // only NFSv4 uses the name, and libnfs leaks the default name it replaces
  if (nfsVersion == 4)
    nfs4_set_client_name(context, NextClientName().c_str());

  CLog::Log(LOGDEBUG, "NFS: version: {}", nfsVersion);
  return nfsVersion;
}

CNfsConnection gNfsConnection;

CNFSFile::CNFSFile() : m_pFileHandle(NULL)
{
  gNfsConnection.AddActiveConnection();
}

CNFSFile::~CNFSFile()
{
  Close();
  gNfsConnection.AddIdleConnection();
}

int CNFSFile::GetChunkSize()
{
  return m_context ? static_cast<int>(m_context->GetMaxReadChunkSize()) : 0;
}

int64_t CNFSFile::GetPosition()
{
  int ret = 0;
  uint64_t offset = 0;

  if (!m_context || m_pFileHandle == NULL)
    return 0;

  std::unique_lock lock(*m_context);
  ret = nfs_lseek(m_context->GetNfsContext(), m_pFileHandle, 0, SEEK_CUR, &offset);

  if (ret < 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to lseek({})", nfs_get_error(m_context->GetNfsContext()));
  }
  return offset;
}

int64_t CNFSFile::GetLength()
{
  if (m_pFileHandle == NULL) return 0;
  return m_fileSize;
}

bool CNFSFile::Open(const CURL& url)
{
  Close();
  // we can't open files like nfs://file.f or nfs://server/file.f
  // if a file matches the if below return false, it can't exist on a nfs share.
  if (!IsValidFile(url.GetFileName()))
  {
    CLog::Log(LOGINFO, "NFS: Bad URL : '{}'", url.GetFileName());
    return false;
  }

  std::string filename;

  m_context = gNfsConnection.GetContext(url, filename);
  if (!m_context)
    return false;

  std::unique_lock lock(*m_context);
  int ret = nfs_open(m_context->GetNfsContext(), filename.c_str(), O_RDONLY, &m_pFileHandle);

  if (ClientDropped(*m_context, ret))
  {
    CLog::Log(LOGERROR,
              "CNFSFile::Open: Unable to open file - trying again with a new context: error: '{}'",
              nfs_get_error(m_context->GetNfsContext()));

    lock.unlock();
    gNfsConnection.RetireContext(*m_context);
    m_context = gNfsConnection.GetContext(url, filename);
    if (!m_context)
      return false;

    lock = std::unique_lock(*m_context);
    ret = nfs_open(m_context->GetNfsContext(), filename.c_str(), O_RDONLY, &m_pFileHandle);
  }

  if (ret != 0)
  {
    CLog::Log(LOGERROR, "CNFSFile::Open: Unable to open file: '{}' error: '{}'", url.GetFileName(),
              nfs_get_error(m_context->GetNfsContext()));

    lock.unlock();
    m_context.Reset();
    return false;
  }
  lock.unlock();

  CLog::Log(LOGDEBUG, "CNFSFile::Open - opened {}", url.GetFileName());
  m_url=url;

  struct __stat64 tmpBuffer;

  // nfs_open succeeds on directories, which lets libdvdread / libbluray
  // mis-classify a folder structure (DVD VIDEO_TS, Bluray BDMV) as a disc image
  // and run UDF/BDMV parsing on it, which then fails. Reject directories here
  // so callers fall back to file-by-file mode, the correct behaviour for folder
  // structures on remote VFS shares.
  if (Stat(&tmpBuffer) != 0 || S_ISDIR(tmpBuffer.st_mode))
  {
    m_url.Reset();
    Close();
    return false;
  }

  m_fileSize = tmpBuffer.st_size;//cache the size of this file
  // We've successfully opened the file!
  return true;
}

bool CNFSFile::Exists(const CURL& url)
{
  return Stat(url,NULL) == 0;
}

int CNFSFile::Stat(struct __stat64* buffer)
{
  if (m_pFileHandle == NULL || !m_context)
    return Stat(m_url, buffer);

  // on the file's own context: a stat by URL could check out, and mount, another one
  nfs_stat_64 tmpBuffer = {};
  std::unique_lock lock(*m_context);
  if (nfs_fstat64(m_context->GetNfsContext(), m_pFileHandle, &tmpBuffer) != 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to stat({}) {}", m_url.GetFileName(),
              nfs_get_error(m_context->GetNfsContext()));
    return -1;
  }
  ToStat64(tmpBuffer, buffer);
  return 0;
}


int CNFSFile::Stat(const CURL& url, struct __stat64* buffer)
{
  int ret = 0;
  std::string filename;

  const CNfsContextLease context = gNfsConnection.GetContext(url, filename);
  if (!context)
    return -1;

  nfs_stat_64 tmpBuffer = {};

  std::unique_lock lock(*context);
  ret = nfs_stat64(context->GetNfsContext(), filename.c_str(), &tmpBuffer);

  //if buffer == NULL we where called from Exists - in that case don't spam the log with errors
  if (ret != 0 && buffer != NULL)
  {
    CLog::Log(LOGERROR, "NFS: Failed to stat({}) {}", url.GetFileName(),
              nfs_get_error(context->GetNfsContext()));
    ret = -1;
  }
  else
  {
    ToStat64(tmpBuffer, buffer);
  }
  return ret;
}

ssize_t CNFSFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  ssize_t numberOfBytesRead = 0;

  if (m_pFileHandle == NULL || !m_context)
    return -1;

  std::unique_lock lock(*m_context);
#ifdef LIBNFS_API_V2
  numberOfBytesRead = nfs_read(m_context->GetNfsContext(), m_pFileHandle, lpBuf, uiBufSize);
#else
  numberOfBytesRead = nfs_read(m_context->GetNfsContext(), m_pFileHandle, uiBufSize, (char*)lpBuf);
#endif

  m_context->resetKeepAlive(m_pFileHandle); //triggers keep alive timer reset for this filehandle

  //something went wrong ...
  if (numberOfBytesRead < 0)
    CLog::Log(LOGERROR, "{} - Error( {}, {} )", __FUNCTION__, (int64_t)numberOfBytesRead,
              nfs_get_error(m_context->GetNfsContext()));

  return numberOfBytesRead;
}

int64_t CNFSFile::Seek(int64_t iFilePosition, int iWhence)
{
  int ret = 0;
  uint64_t offset = 0;

  if (m_pFileHandle == NULL || !m_context)
    return -1;
  std::unique_lock lock(*m_context);

  ret = nfs_lseek(m_context->GetNfsContext(), m_pFileHandle, iFilePosition, iWhence, &offset);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "{} - Error( seekpos: {}, whence: {}, fsize: {}, {})", __FUNCTION__,
              iFilePosition, iWhence, m_fileSize, nfs_get_error(m_context->GetNfsContext()));
    return -1;
  }
  return (int64_t)offset;
}

int CNFSFile::Truncate(int64_t iSize)
{
  int ret = 0;

  if (m_pFileHandle == NULL || !m_context)
    return -1;
  std::unique_lock lock(*m_context);

  ret = nfs_ftruncate(m_context->GetNfsContext(), m_pFileHandle, iSize);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "{} - Error( ftruncate: {}, fsize: {}, {})", __FUNCTION__, iSize,
              m_fileSize, nfs_get_error(m_context->GetNfsContext()));
    return -1;
  }
  return ret;
}

void CNFSFile::Close()
{
  if (m_pFileHandle != NULL && m_context)
  {
    std::unique_lock lock(*m_context);
    int ret = 0;
    CLog::Log(LOGDEBUG, "CNFSFile::Close closing file {}", m_url.GetFileName());
    // remove it from keep alive list before closing
    // so keep alive code doesn't process it anymore
    m_context->removeFromKeepAliveList(m_pFileHandle);
    ret = nfs_close(m_context->GetNfsContext(), m_pFileHandle);

    if (ret < 0)
    {
      CLog::Log(LOGERROR, "Failed to close({}) - {}", m_url.GetFileName(),
                nfs_get_error(m_context->GetNfsContext()));
    }
    m_pFileHandle = NULL;
    m_fileSize = 0;
  }
  m_context.Reset();
}

//this was a bitch!
//for nfs write to work we have to write chunked
//otherwise this could crash on big files
ssize_t CNFSFile::Write(const void* lpBuf, size_t uiBufSize)
{
  size_t numberOfBytesWritten = 0;
  int writtenBytes = 0;
  size_t leftBytes = uiBufSize;

  if (m_pFileHandle == NULL || !m_context)
    return -1;

  //clamp max write chunksize to 32kb - fixme - this might be superfluous with future libnfs versions
  size_t chunkSize = m_context->GetMaxWriteChunkSize() > 32768
                         ? 32768
                         : static_cast<size_t>(m_context->GetMaxWriteChunkSize());

  std::unique_lock lock(*m_context);

  //write as long as some bytes are left to be written
  while( leftBytes )
  {
    //the last chunk could be smalle than chunksize
    if(leftBytes < chunkSize)
    {
      chunkSize = leftBytes;//write last chunk with correct size
    }
#ifdef LIBNFS_API_V2
    writtenBytes = nfs_write(m_context->GetNfsContext(), m_pFileHandle,
                             static_cast<const char*>(lpBuf) + numberOfBytesWritten, chunkSize);
#else
    //write chunk
    //! @bug libnfs < 2.0.0 isn't const correct
    writtenBytes = nfs_write(m_context->GetNfsContext(), m_pFileHandle, chunkSize,
                             const_cast<char*>((const char*)lpBuf) + numberOfBytesWritten);
#endif
    //decrease left bytes
    leftBytes-= writtenBytes;
    //increase overall written bytes
    numberOfBytesWritten += writtenBytes;

    //danger - something went wrong
    if (writtenBytes < 0)
    {
      CLog::Log(LOGERROR, "Failed to pwrite({}) {}", m_url.GetFileName(),
                nfs_get_error(m_context->GetNfsContext()));
      if (numberOfBytesWritten == 0)
        return -1;

      break;
    }
  }
  //return total number of written bytes
  return numberOfBytesWritten;
}

bool CNFSFile::Delete(const CURL& url)
{
  int ret = 0;
  std::string filename;

  const CNfsContextLease context = gNfsConnection.GetContext(url, filename);
  if (!context)
    return false;

  std::unique_lock lock(*context);
  ret = nfs_unlink(context->GetNfsContext(), filename.c_str());

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, nfs_get_error(context->GetNfsContext()));
  }
  return (ret == 0);
}

bool CNFSFile::Rename(const CURL& url, const CURL& urlnew)
{
  int ret = 0;
  std::string strFile;

  const CNfsContextLease context = gNfsConnection.GetContext(url, strFile);
  if (!context)
    return false;

  std::string strFileNew;
  std::string strDummy;
  gNfsConnection.splitUrlIntoExportAndPath(urlnew, strDummy, strFileNew);

  std::unique_lock lock(*context);
  ret = nfs_rename(context->GetNfsContext(), strFile.c_str(), strFileNew.c_str());

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, nfs_get_error(context->GetNfsContext()));
  }
  return (ret == 0);
}

bool CNFSFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  int ret = 0;
  // we can't open files like nfs://file.f or nfs://server/file.f
  // if a file matches the if below return false, it can't exist on a nfs share.
  if (!IsValidFile(url.GetFileName())) return false;

  Close();
  std::string filename;

  m_context = gNfsConnection.GetContext(url, filename);
  if (!m_context)
    return false;

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileNFS::OpenForWrite() called with overwriting enabled! - {}",
              filename);
  }

  // nfs_open2 handles both creation and open atomically;
  const int flags = bOverWrite ? O_CREAT | O_RDWR | O_EXCL : O_RDWR;
  const int mode = bOverWrite ? S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH : 0;

  // taken before the context lock, so that a waiting create holds up no other user of its context
  std::unique_lock createLock(nfs4CreateLock, std::defer_lock);
  if (bOverWrite && m_context->GetNfsVersion() == 4)
    createLock.lock();

  std::unique_lock lock(*m_context);
  ret = nfs_open2(m_context->GetNfsContext(), filename.c_str(), flags, mode, &m_pFileHandle);

  // as when an otherwise idle context waits behind slow creates until its lease expires
  if (ClientDropped(*m_context, ret))
  {
    CLog::LogF(LOGERROR, "Unable to open file - trying again with a new context: error: '{}'",
               nfs_get_error(m_context->GetNfsContext()));

    lock.unlock();
    if (createLock.owns_lock())
      createLock.unlock();
    gNfsConnection.RetireContext(*m_context);
    m_context = gNfsConnection.GetContext(url, filename);
    if (!m_context)
      return false;

    if (bOverWrite && m_context->GetNfsVersion() == 4)
      createLock.lock();
    lock = std::unique_lock(*m_context);
    ret = nfs_open2(m_context->GetNfsContext(), filename.c_str(), flags, mode, &m_pFileHandle);
  }

  if (ret || m_pFileHandle == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "CNFSFile::Open: Unable to open file : '{}' error : '{}'", filename,
              nfs_get_error(m_context->GetNfsContext()));
    lock.unlock();
    m_context.Reset();
    return false;
  }
  lock.unlock();
  m_url=url;

  struct __stat64 tmpBuffer = {};

  //only stat if file was not created
  if(!bOverWrite)
  {
    if(Stat(&tmpBuffer))
    {
      m_url.Reset();
      Close();
      return false;
    }
    m_fileSize = tmpBuffer.st_size;//cache filesize of this file
  }
  else//file was created - filesize is zero
  {
    m_fileSize = 0;
  }

  // We've successfully opened the file!
  return true;
}

bool CNFSFile::IsValidFile(const std::string& strFileName)
{
  if (strFileName.find('/') == std::string::npos || /* doesn't have sharename */
      StringUtils::EndsWith(strFileName, "/.") || /* not current folder */
      StringUtils::EndsWith(strFileName, "/.."))  /* not parent folder */
    return false;
  return true;
}
