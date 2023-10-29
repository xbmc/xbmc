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
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <inttypes.h>
#include <mutex>

#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs.h>

#ifdef TARGET_WINDOWS
#include <fcntl.h>
#include <sys\stat.h>
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
constexpr auto IDLE_TIMEOUT = 30s; // close fast unused contexts when no active connections

constexpr int NFS4ERR_EXPIRED = -11; // client session expired due idle time greater than lease_time

constexpr auto SETTING_NFS_VERSION = "nfs.version";
constexpr auto SETTING_NFS_CHUNKSIZE = "nfs.chunksize";
} // unnamed namespace

CNfsConnection::CNfsConnection()
  : m_pNfsContext(NULL),
    m_exportPath(""),
    m_hostName(""),
    m_resolvedHostName(""),
    m_IdleTimeout(std::chrono::steady_clock::now() + IDLE_TIMEOUT)
{
}

CNfsConnection::~CNfsConnection()
{
  Deinit();
}

void CNfsConnection::resolveHost(const CURL& url)
{
  // resolve if hostname has changed
  CDNSNameCache::Lookup(url.GetHostName(), m_resolvedHostName);
}

std::list<std::string> CNfsConnection::GetExportList(const CURL& url)
{
  std::list<std::string> retList;

  struct exportnode *exportlist, *tmp;
#ifdef HAS_NFS_MOUNT_GETEXPORTS_TIMEOUT
  exportlist = mount_getexports_timeout(
      m_resolvedHostName.c_str(),
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_nfsTimeout * 1000);
#else
  exportlist = mount_getexports(m_resolvedHostName.c_str());
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

void CNfsConnection::clearMembers()
{
    // NOTE - DON'T CLEAR m_exportList HERE!
    // splitUrlIntoExportAndPath checks for m_exportList.empty()
    // and would query the server in an excessive unwanted fashion
    // also don't clear m_KeepAliveTimeouts here because we
    // would loose any "paused" file handles during export change
    m_exportPath.clear();
    m_hostName.clear();
    m_writeChunkSize = 0;
    m_readChunkSize = 0;
    m_pNfsContext = NULL;
}

void CNfsConnection::destroyOpenContexts()
{
  std::unique_lock<CCriticalSection> lock(openContextLock);
  for (auto& it : m_openContextMap)
  {
    nfs_destroy_context(it.second.pContext);
  }
  m_openContextMap.clear();
}

void CNfsConnection::destroyContext(const std::string &exportName)
{
  std::unique_lock<CCriticalSection> lock(openContextLock);
  tOpenContextMap::iterator it = m_openContextMap.find(exportName.c_str());
  if (it != m_openContextMap.end())
  {
    nfs_destroy_context(it->second.pContext);
    m_openContextMap.erase(it);
  }
}

struct nfs_context *CNfsConnection::getContextFromMap(const std::string &exportname, bool forceCacheHit/* = false*/)
{
  struct nfs_context *pRet = NULL;
  std::unique_lock<CCriticalSection> lock(openContextLock);

  tOpenContextMap::iterator it = m_openContextMap.find(exportname.c_str());
  if (it != m_openContextMap.end())
  {
    //check if context has timed out already
    auto now = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastAccessedTime);
    if (duration < CONTEXT_TIMEOUT || forceCacheHit)
    {
      //its not timedout yet or caller wants the cached entry regardless of timeout
      //refresh access time of that
      //context and return it
      if (!forceCacheHit) // only log it if this isn't the resetkeepalive on each read ;)
        CLog::Log(LOGDEBUG, "NFS: Refreshing context for {}, old: {}, new: {}", exportname,
                  it->second.lastAccessedTime.time_since_epoch().count(),
                  now.time_since_epoch().count());
      it->second.lastAccessedTime = now;
      pRet = it->second.pContext;
    }
    else
    {
      //context is timed out
      //destroy it and return NULL
      CLog::Log(LOGDEBUG, "NFS: Old context timed out - destroying it");
      nfs_destroy_context(it->second.pContext);
      m_openContextMap.erase(it);
    }
  }
  return pRet;
}

CNfsConnection::ContextStatus CNfsConnection::getContextForExport(const std::string& exportname)
{
  CNfsConnection::ContextStatus ret = CNfsConnection::ContextStatus::INVALID;

  clearMembers();

  m_pNfsContext = getContextFromMap(exportname);

  if(!m_pNfsContext)
  {
    CLog::Log(LOGDEBUG, "NFS: Context for {} not open - get a new context.", exportname);
    m_pNfsContext = nfs_init_context();

    if(!m_pNfsContext)
    {
      CLog::Log(LOGERROR,"NFS: Error initcontext in getContextForExport.");
    }
    else
    {
      struct contextTimeout tmp;
      std::unique_lock<CCriticalSection> lock(openContextLock);
      setOptions(m_pNfsContext);
      tmp.pContext = m_pNfsContext;
      tmp.lastAccessedTime = std::chrono::steady_clock::now();
      m_openContextMap[exportname] = tmp; //add context to list of all contexts
      ret = CNfsConnection::ContextStatus::NEW;
    }
  }
  else
  {
    ret = CNfsConnection::ContextStatus::CACHED;
    CLog::Log(LOGDEBUG,"NFS: Using cached context.");
  }
  m_lastAccessedTime = std::chrono::steady_clock::now();

  return ret;
}

bool CNfsConnection::splitUrlIntoExportAndPath(const CURL& url, std::string &exportPath, std::string &relativePath)
{
  //refresh exportlist if empty or hostname change
  if(m_exportList.empty() || !StringUtils::EqualsNoCase(url.GetHostName(), m_hostName))
  {
    const auto settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return false;

    const auto settings = settingsComponent->GetSettings();
    if (!settings)
      return false;

    const int nfsVersion = settings->GetInt(SETTING_NFS_VERSION);

    if (nfsVersion == 4)
      m_exportList = {"/"};
    else
      m_exportList = GetExportList(url);
  }

  return splitUrlIntoExportAndPath(url, exportPath, relativePath, m_exportList);
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

bool CNfsConnection::Connect(const CURL& url, std::string &relativePath)
{
  std::unique_lock<CCriticalSection> lock(*this);
  int nfsRet = 0;
  std::string exportPath;

  resolveHost(url);
  bool ret = splitUrlIntoExportAndPath(url, exportPath, relativePath);

  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastAccessedTime);

  if ((ret && (exportPath != m_exportPath || url.GetHostName() != m_hostName)) ||
      duration > CONTEXT_TIMEOUT)
  {
    CNfsConnection::ContextStatus contextRet = getContextForExport(url.GetHostName() + exportPath);

    // we need a new context because sharename or hostname has changed
    if (contextRet == CNfsConnection::ContextStatus::INVALID)
    {
      return false;
    }

    // new context was created - we need to mount it
    if (contextRet == CNfsConnection::ContextStatus::NEW)
    {
      //we connect to the directory of the path. This will be the "root" path of this connection then.
      //So all fileoperations are relative to this mountpoint...
      nfsRet = nfs_mount(m_pNfsContext, m_resolvedHostName.c_str(), exportPath.c_str());

      if(nfsRet != 0)
      {
        CLog::Log(LOGERROR, "NFS: Failed to mount nfs share: {} ({})", exportPath,
                  nfs_get_error(m_pNfsContext));
        destroyContext(url.GetHostName() + exportPath);
        return false;
      }
      CLog::Log(LOGDEBUG, "NFS: Connected to server {} and export {}", url.GetHostName(),
                exportPath);
    }
    m_exportPath = exportPath;
    m_hostName = url.GetHostName();

    // read chunksize only works after mount
    m_readChunkSize = nfs_get_readmax(m_pNfsContext);
    m_writeChunkSize = nfs_get_writemax(m_pNfsContext);

    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    const uint64_t chunkSize =
        settings ? (settings->GetInt(SETTING_NFS_CHUNKSIZE) * 1024) : (128 * 1024);

    if (m_readChunkSize == 0)
    {
      CLog::Log(LOGDEBUG, "NFS Server did not return max read chunksize - Using setting value {}",
                chunkSize);
      m_readChunkSize = chunkSize;
    }
    else if (chunkSize < m_readChunkSize)
    {
      CLog::Log(LOGDEBUG,
                "NFS Server max read chunksize ({}) is bigger than client setting - Using client "
                "value {}",
                m_readChunkSize, chunkSize);
      m_readChunkSize = chunkSize;
    }

    if (m_writeChunkSize == 0)
    {
      CLog::Log(LOGDEBUG, "NFS Server did not return max write chunksize - Using setting value {}",
                chunkSize);
      m_writeChunkSize = chunkSize;
    }
    else if (chunkSize < m_writeChunkSize)
    {
      CLog::Log(LOGDEBUG,
                "NFS Server max write chunksize ({}) is bigger than client setting - Using client "
                "value {}",
                m_writeChunkSize, chunkSize);
      m_writeChunkSize = chunkSize;
    }

    if (contextRet == CNfsConnection::ContextStatus::NEW)
    {
      CLog::Log(LOGDEBUG, "NFS: chunks: r/w {}/{}", (int)m_readChunkSize, (int)m_writeChunkSize);
    }
  }
  return ret;
}

void CNfsConnection::Deinit()
{
  if(m_pNfsContext)
  {
    destroyOpenContexts();
    m_pNfsContext = NULL;
  }
  clearMembers();
  // clear any keep alive timeouts on deinit
  m_KeepAliveTimeouts.clear();
}

/* This is called from CApplication::ProcessSlow() and is used to tell if nfs have been idle for too long */
void CNfsConnection::CheckIfIdle()
{
  /* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if which will lead to another check, which is locked.  */
  if (m_OpenConnections == 0 && m_pNfsContext != NULL)
  { /* I've set the the maximum IDLE time to be 1 min and 30 sec. */
    std::unique_lock<CCriticalSection> lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */)
    {
      const auto now = std::chrono::steady_clock::now();

      if (m_IdleTimeout < now)
      {
        CLog::Log(LOGINFO, "NFS is idle. Closing the remaining connections.");
        gNfsConnection.Deinit();
      }
    }
  }

  if( m_pNfsContext != NULL )
  {
    std::unique_lock<CCriticalSection> lock(keepAliveLock);

    const auto now = std::chrono::steady_clock::now();

    //handle keep alive on opened files
    for (auto& it : m_KeepAliveTimeouts)
    {
      if (it.second.refreshTime < now)
      {
        keepAlive(it.second.exportPath, it.first);
        //reset timeout
        resetKeepAlive(it.second.exportPath, it.first);
      }
    }
  }
}

//remove file handle from keep alive list on file close
void CNfsConnection::removeFromKeepAliveList(struct nfsfh  *_pFileHandle)
{
  std::unique_lock<CCriticalSection> lock(keepAliveLock);
  m_KeepAliveTimeouts.erase(_pFileHandle);
}

//reset timeouts on read
void CNfsConnection::resetKeepAlive(const std::string& _exportPath, struct nfsfh* _pFileHandle)
{
  std::unique_lock<CCriticalSection> lock(keepAliveLock);
  //refresh last access time of the context aswell
  struct nfs_context *pContext = getContextFromMap(_exportPath, true);

  // if we keep alive using m_pNfsContext we need to mark
  // its last access time too here
  if (m_pNfsContext == pContext)
  {
    m_lastAccessedTime = std::chrono::steady_clock::now();
  }

  //adds new keys - refreshes existing ones
  m_KeepAliveTimeouts[_pFileHandle].exportPath = _exportPath;
  m_KeepAliveTimeouts[_pFileHandle].refreshTime = m_lastAccessedTime + KEEP_ALIVE_TIMEOUT;
}

//keep alive the filehandles nfs connection
//by blindly doing a read 32bytes - seek back to where
//we were before
void CNfsConnection::keepAlive(const std::string& _exportPath, struct nfsfh* _pFileHandle)
{
  uint64_t offset = 0;
  char buffer[32];
  // this also refreshes the last accessed time for the context
  // true forces a cachehit regardless the context is timedout
  // on this call we are sure its not timedout even if the last accessed
  // time suggests it.
  struct nfs_context *pContext = getContextFromMap(_exportPath, true);

  if (!pContext)// this should normally never happen - paranoia
    pContext = m_pNfsContext;

  CLog::LogF(LOGDEBUG, "sending keep alive after {}s.",
             std::chrono::duration_cast<std::chrono::seconds>(KEEP_ALIVE_TIMEOUT).count());

  std::unique_lock<CCriticalSection> lock(*this);

  nfs_lseek(pContext, _pFileHandle, 0, SEEK_CUR, &offset);

  int bytes = nfs_read(pContext, _pFileHandle, 32, buffer);
  if (bytes < 0)
  {
    CLog::LogF(LOGERROR, "nfs_read - Error ({}, {})", bytes, nfs_get_error(pContext));
    return;
  }

  nfs_lseek(pContext, _pFileHandle, offset, SEEK_SET, &offset);
}

int CNfsConnection::stat(const CURL& url, nfs_stat_64* statbuff)
{
  std::unique_lock<CCriticalSection> lock(*this);
  int nfsRet = 0;
  std::string exportPath;
  std::string relativePath;
  struct nfs_context *pTmpContext = NULL;

  resolveHost(url);

  if(splitUrlIntoExportAndPath(url, exportPath, relativePath))
  {
    pTmpContext = nfs_init_context();

    if(pTmpContext)
    {
      setOptions(pTmpContext);
      //we connect to the directory of the path. This will be the "root" path of this connection then.
      //So all fileoperations are relative to this mountpoint...
      nfsRet = nfs_mount(pTmpContext, m_resolvedHostName.c_str(), exportPath.c_str());

      if(nfsRet == 0)
      {
        nfsRet = nfs_stat64(pTmpContext, relativePath.c_str(), statbuff);
      }
      else
      {
        CLog::Log(LOGERROR, "NFS: Failed to mount nfs share: {} ({})", exportPath,
                  nfs_get_error(m_pNfsContext));
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
  std::unique_lock<CCriticalSection> lock(*this);
  m_OpenConnections++;
}

void CNfsConnection::AddIdleConnection()
{
  std::unique_lock<CCriticalSection> lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any weird behaviours if a user
   leaves the movie paused for a long while and then press stop */
  const auto now = std::chrono::steady_clock::now();
  m_IdleTimeout = now + IDLE_TIMEOUT;
}


void CNfsConnection::setOptions(struct nfs_context* context)
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const auto advancedSettings = settingsComponent->GetAdvancedSettings();
  if (!advancedSettings)
    return;

#ifdef HAS_NFS_SET_TIMEOUT
  uint32_t timeout = advancedSettings->m_nfsTimeout;
  nfs_set_timeout(context, timeout > 0 ? timeout * 1000 : -1);
#endif
  int retries = advancedSettings->m_nfsRetries;
  nfs_set_autoreconnect(context, retries);

  const auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  const int nfsVersion = settings->GetInt(SETTING_NFS_VERSION);

  int ret = nfs_set_version(context, nfsVersion);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to set nfs version: {} ({})", nfsVersion,
              nfs_get_error(context));
    return;
  }

  CLog::Log(LOGDEBUG, "NFS: version: {}", nfsVersion);
}

CNfsConnection gNfsConnection;

CNFSFile::CNFSFile()
: m_pFileHandle(NULL)
, m_pNfsContext(NULL)
{
  gNfsConnection.AddActiveConnection();
}

CNFSFile::~CNFSFile()
{
  Close();
  gNfsConnection.AddIdleConnection();
}

int64_t CNFSFile::GetPosition()
{
  int ret = 0;
  uint64_t offset = 0;
  std::unique_lock<CCriticalSection> lock(gNfsConnection);

  if (gNfsConnection.GetNfsContext() == NULL || m_pFileHandle == NULL) return 0;

  ret = nfs_lseek(gNfsConnection.GetNfsContext(), m_pFileHandle, 0, SEEK_CUR, &offset);

  if (ret < 0)
  {
    CLog::Log(LOGERROR, "NFS: Failed to lseek({})", nfs_get_error(gNfsConnection.GetNfsContext()));
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

  std::unique_lock<CCriticalSection> lock(gNfsConnection);

  if (!gNfsConnection.Connect(url, filename))
    return false;

  m_pNfsContext = gNfsConnection.GetNfsContext();
  m_exportPath = gNfsConnection.GetContextMapId();

  int ret = nfs_open(m_pNfsContext, filename.c_str(), O_RDONLY, &m_pFileHandle);

  if (ret == NFS4ERR_EXPIRED) // client session expired due no activity/keep alive
  {
    CLog::Log(LOGERROR,
              "CNFSFile::Open: Unable to open file - trying again with a new context: error: '{}'",
              nfs_get_error(m_pNfsContext));

    gNfsConnection.Deinit();
    m_pNfsContext = gNfsConnection.GetNfsContext();
    m_exportPath = gNfsConnection.GetContextMapId();
    ret = nfs_open(m_pNfsContext, filename.c_str(), O_RDONLY, &m_pFileHandle);
  }

  if (ret != 0)
  {
    CLog::Log(LOGERROR, "CNFSFile::Open: Unable to open file: '{}' error: '{}'", url.GetFileName(),
              nfs_get_error(m_pNfsContext));

    m_pNfsContext = nullptr;
    m_exportPath.clear();
    return false;
  }

  CLog::Log(LOGDEBUG, "CNFSFile::Open - opened {}", url.GetFileName());
  m_url=url;

  struct __stat64 tmpBuffer;

  if( Stat(&tmpBuffer) )
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
  return Stat(m_url,buffer);
}


int CNFSFile::Stat(const CURL& url, struct __stat64* buffer)
{
  int ret = 0;
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string filename;

  if(!gNfsConnection.Connect(url,filename))
    return -1;

  nfs_stat_64 tmpBuffer = {};

  ret = nfs_stat64(gNfsConnection.GetNfsContext(), filename.c_str(), &tmpBuffer);

  //if buffer == NULL we where called from Exists - in that case don't spam the log with errors
  if (ret != 0 && buffer != NULL)
  {
    CLog::Log(LOGERROR, "NFS: Failed to stat({}) {}", url.GetFileName(),
              nfs_get_error(gNfsConnection.GetNfsContext()));
    ret = -1;
  }
  else
  {
    if (buffer)
    {
      *buffer = {};
      buffer->st_dev = tmpBuffer.nfs_dev;
      buffer->st_ino = tmpBuffer.nfs_ino;
      buffer->st_mode = tmpBuffer.nfs_mode;
      buffer->st_nlink = tmpBuffer.nfs_nlink;
      buffer->st_uid = tmpBuffer.nfs_uid;
      buffer->st_gid = tmpBuffer.nfs_gid;
      buffer->st_rdev = tmpBuffer.nfs_rdev;
      buffer->st_size = tmpBuffer.nfs_size;
      buffer->st_atime = tmpBuffer.nfs_atime;
      buffer->st_mtime = tmpBuffer.nfs_mtime;
      buffer->st_ctime = tmpBuffer.nfs_ctime;
    }
  }
  return ret;
}

ssize_t CNFSFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  ssize_t numberOfBytesRead = 0;
  std::unique_lock<CCriticalSection> lock(gNfsConnection);

  if (m_pFileHandle == NULL || m_pNfsContext == NULL )
    return -1;

  numberOfBytesRead = nfs_read(m_pNfsContext, m_pFileHandle, uiBufSize, (char *)lpBuf);

  lock.unlock(); //no need to keep the connection lock after that

  gNfsConnection.resetKeepAlive(m_exportPath, m_pFileHandle);//triggers keep alive timer reset for this filehandle

  //something went wrong ...
  if (numberOfBytesRead < 0)
    CLog::Log(LOGERROR, "{} - Error( {}, {} )", __FUNCTION__, (int64_t)numberOfBytesRead,
              nfs_get_error(m_pNfsContext));

  return numberOfBytesRead;
}

int64_t CNFSFile::Seek(int64_t iFilePosition, int iWhence)
{
  int ret = 0;
  uint64_t offset = 0;

  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  if (m_pFileHandle == NULL || m_pNfsContext == NULL) return -1;


  ret = nfs_lseek(m_pNfsContext, m_pFileHandle, iFilePosition, iWhence, &offset);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "{} - Error( seekpos: {}, whence: {}, fsize: {}, {})", __FUNCTION__,
              iFilePosition, iWhence, m_fileSize, nfs_get_error(m_pNfsContext));
    return -1;
  }
  return (int64_t)offset;
}

int CNFSFile::Truncate(int64_t iSize)
{
  int ret = 0;

  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  if (m_pFileHandle == NULL || m_pNfsContext == NULL) return -1;


  ret = nfs_ftruncate(m_pNfsContext, m_pFileHandle, iSize);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "{} - Error( ftruncate: {}, fsize: {}, {})", __FUNCTION__, iSize,
              m_fileSize, nfs_get_error(m_pNfsContext));
    return -1;
  }
  return ret;
}

void CNFSFile::Close()
{
  std::unique_lock<CCriticalSection> lock(gNfsConnection);

  if (m_pFileHandle != NULL && m_pNfsContext != NULL)
  {
    int ret = 0;
    CLog::Log(LOGDEBUG, "CNFSFile::Close closing file {}", m_url.GetFileName());
    // remove it from keep alive list before closing
    // so keep alive code doesn't process it anymore
    gNfsConnection.removeFromKeepAliveList(m_pFileHandle);
    ret = nfs_close(m_pNfsContext, m_pFileHandle);

	  if (ret < 0)
    {
      CLog::Log(LOGERROR, "Failed to close({}) - {}", m_url.GetFileName(),
                nfs_get_error(m_pNfsContext));
    }
    m_pFileHandle = NULL;
    m_pNfsContext = NULL;
    m_fileSize = 0;
    m_exportPath.clear();
  }
}

//this was a bitch!
//for nfs write to work we have to write chunked
//otherwise this could crash on big files
ssize_t CNFSFile::Write(const void* lpBuf, size_t uiBufSize)
{
  size_t numberOfBytesWritten = 0;
  int writtenBytes = 0;
  size_t leftBytes = uiBufSize;
  //clamp max write chunksize to 32kb - fixme - this might be superfluous with future libnfs versions
  size_t chunkSize = gNfsConnection.GetMaxWriteChunkSize() > 32768 ? 32768 : (size_t)gNfsConnection.GetMaxWriteChunkSize();

  std::unique_lock<CCriticalSection> lock(gNfsConnection);

  if (m_pFileHandle == NULL || m_pNfsContext == NULL) return -1;

  //write as long as some bytes are left to be written
  while( leftBytes )
  {
    //the last chunk could be smalle than chunksize
    if(leftBytes < chunkSize)
    {
      chunkSize = leftBytes;//write last chunk with correct size
    }
    //write chunk
    //! @bug libnfs < 2.0.0 isn't const correct
    writtenBytes = nfs_write(m_pNfsContext,
                                  m_pFileHandle,
                                  chunkSize,
                                  const_cast<char*>((const char *)lpBuf) + numberOfBytesWritten);
    //decrease left bytes
    leftBytes-= writtenBytes;
    //increase overall written bytes
    numberOfBytesWritten += writtenBytes;

    //danger - something went wrong
    if (writtenBytes < 0)
    {
      CLog::Log(LOGERROR, "Failed to pwrite({}) {}", m_url.GetFileName(),
                nfs_get_error(m_pNfsContext));
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
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string filename;

  if(!gNfsConnection.Connect(url, filename))
    return false;


  ret = nfs_unlink(gNfsConnection.GetNfsContext(), filename.c_str());

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__,
              nfs_get_error(gNfsConnection.GetNfsContext()));
  }
  return (ret == 0);
}

bool CNFSFile::Rename(const CURL& url, const CURL& urlnew)
{
  int ret = 0;
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string strFile;

  if(!gNfsConnection.Connect(url,strFile))
    return false;

  std::string strFileNew;
  std::string strDummy;
  gNfsConnection.splitUrlIntoExportAndPath(urlnew, strDummy, strFileNew);

  ret = nfs_rename(gNfsConnection.GetNfsContext() , strFile.c_str(), strFileNew.c_str());

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__,
              nfs_get_error(gNfsConnection.GetNfsContext()));
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
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string filename;

  if(!gNfsConnection.Connect(url,filename))
    return false;

  m_pNfsContext = gNfsConnection.GetNfsContext();
  m_exportPath = gNfsConnection.GetContextMapId();

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileNFS::OpenForWrite() called with overwriting enabled! - {}",
              filename);
    //create file with proper permissions
    ret = nfs_creat(m_pNfsContext, filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &m_pFileHandle);
    //if file was created the file handle isn't valid ... so close it and open later
    if(ret == 0)
    {
      nfs_close(m_pNfsContext,m_pFileHandle);
      m_pFileHandle = NULL;
    }
  }

  ret = nfs_open(m_pNfsContext, filename.c_str(), O_RDWR, &m_pFileHandle);

  if (ret || m_pFileHandle == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "CNFSFile::Open: Unable to open file : '{}' error : '{}'", filename,
              nfs_get_error(gNfsConnection.GetNfsContext()));
    m_pNfsContext = NULL;
    m_exportPath.clear();
    return false;
  }
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
