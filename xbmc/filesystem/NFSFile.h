/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// FileNFS.h: interface for the CNFSFile class.

#include "IFile.h"
#include "URL.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct nfs_context;
struct nfsfh;
struct nfs_stat_64;

class CNfsContextPool;

// One mounted nfs_context. Lock it around every libnfs call on GetNfsContext().
class CNfsContext : public CCriticalSection
{
public:
  CNfsContext(std::string key,
              std::string resolvedHostName,
              std::string exportPath,
              struct nfs_context* pNfsContext);
  ~CNfsContext();
  CNfsContext(const CNfsContext&) = delete;
  CNfsContext& operator=(const CNfsContext&) = delete;

  struct nfs_context* GetNfsContext() const { return m_pNfsContext; }
  const std::string& GetConnectedIp() const { return m_resolvedHostName; }
  const std::string& GetConnectedExport() const { return m_exportPath; }
  uint64_t GetMaxReadChunkSize() const { return m_readChunkSize; }
  uint64_t GetMaxWriteChunkSize() const { return m_writeChunkSize; }
  int GetNfsVersion() const { return m_nfsVersion; }

  // The keep alive list may only be changed while holding the context lock.
  //adds the filehandle to the keep alive list or resets
  //the timeout for this filehandle if already in list
  void resetKeepAlive(struct nfsfh* pFileHandle);
  //removes file handle from keep alive list
  void removeFromKeepAliveList(struct nfsfh* pFileHandle);

private:
  friend class CNfsConnection;
  friend class CNfsContextPool;

  enum class MountState
  {
    NONE,
    MOUNTED,
    FAILED
  };

  void Touch(std::chrono::time_point<std::chrono::steady_clock> now);
  bool IsExpired(std::chrono::time_point<std::chrono::steady_clock> now,
                 std::chrono::steady_clock::duration timeout) const;
  std::vector<struct nfsfh*> GetDueKeepAlives(
      std::chrono::time_point<std::chrono::steady_clock> now) const;

  const std::string m_key; // key in CNfsContextPool
  const std::string m_resolvedHostName;
  const std::string m_exportPath;
  struct nfs_context* const m_pNfsContext;

  // written while mounting, under the context lock
  MountState m_mountState = MountState::NONE;
  uint64_t m_readChunkSize = 0;
  uint64_t m_writeChunkSize = 0;
  int m_nfsVersion = 3;

  // guarded by the pool's lock
  unsigned int m_users = 0;
  bool m_retired = false;

  mutable CCriticalSection m_stateLock; // guards the two members below, never held across I/O
  std::chrono::time_point<std::chrono::steady_clock> m_lastUsed;
  std::map<struct nfsfh*, std::chrono::time_point<std::chrono::steady_clock>> m_keepAliveTimeouts;
};

// One use of a context, by an open file or for a single call. The context is not expired while
// it has users.
class CNfsContextLease
{
public:
  CNfsContextLease() = default;
  CNfsContextLease(CNfsContextPool& pool, std::shared_ptr<CNfsContext> context);
  ~CNfsContextLease();
  CNfsContextLease(CNfsContextLease&& other) noexcept;
  CNfsContextLease& operator=(CNfsContextLease&& other) noexcept;
  CNfsContextLease(const CNfsContextLease&) = delete;
  CNfsContextLease& operator=(const CNfsContextLease&) = delete;

  void Reset();
  explicit operator bool() const { return m_context != nullptr; }
  CNfsContext& operator*() const { return *m_context; }
  CNfsContext* operator->() const { return m_context.get(); }

private:
  CNfsContextPool* m_pool = nullptr;
  std::shared_ptr<CNfsContext> m_context;
};

// The contexts of each export and their users. Its lock is never held across network I/O or
// while waiting for a context lock; it may be taken while holding a context lock.
class CNfsContextPool
{
public:
  using Factory = std::function<std::shared_ptr<CNfsContext>()>;
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

  // Takes a use of a context for key: one without users, else a new one from create(), called
  // under the pool lock, while key has fewer than maxContexts, else the one with fewest users.
  // Expired contexts are replaced.
  CNfsContextLease Checkout(const std::string& key,
                            unsigned int maxContexts,
                            const Factory& create);

  // Stops handing context out. It is dropped once its last user releases it.
  void Retire(const CNfsContext& context);
  void RetireAll();

  // Removes the contexts without users that have expired, or every one without users if all.
  // An export's first context expires after CONTEXT_TIMEOUT, its others after IDLE_TIMEOUT.
  std::vector<std::shared_ptr<CNfsContext>> TakeUnused(TimePoint now, bool all);

  // Contexts, retired ones included, with a keep alive due.
  std::vector<std::shared_ptr<CNfsContext>> GetKeepAlivesDue(TimePoint now) const;

private:
  friend class CNfsContextLease;
  void Release(CNfsContext& context);

  mutable CCriticalSection m_lock;
  // host + export -> contexts, oldest first
  std::map<std::string, std::vector<std::shared_ptr<CNfsContext>>> m_contexts;
  std::vector<std::shared_ptr<CNfsContext>> m_retired; // until their last user leaves
};

class CNfsConnection
{
public:
  CNfsConnection();
  ~CNfsConnection();
  // returns the mounted context to use for url and the path relative to its export
  CNfsContextLease GetContext(const CURL& url, std::string& relativePath);
  // no longer hands context out, its users keep it until they release it
  void RetireContext(const CNfsContext& context);
  std::list<std::string> GetExportList(const CURL &url);
  //this functions splits the url into the exportpath (feed to mount) and the rest of the path
  //relative to the mounted export
  bool splitUrlIntoExportAndPath(const CURL& url, std::string &exportPath, std::string &relativePath, std::list<std::string> &exportList);
  bool splitUrlIntoExportAndPath(const CURL& url, std::string &exportPath, std::string &relativePath);

  //special stat which uses its own context
  //needed for getting intervolume symlinks to work
  int stat(const CURL& url, nfs_stat_64* statbuff);

  void AddActiveConnection();
  void AddIdleConnection();
  void CheckIfIdle();
  void Deinit();

private:
  class CMaintenanceJob;

  CNfsContextPool m_pool;
  std::atomic<bool> m_maintenancePending{false};

  // Guards the members below. Never held across network I/O or while waiting for a context
  // lock; it may be taken while holding a context lock.
  CCriticalSection m_lock;
  int m_OpenConnections = 0; //number of open connections
  std::chrono::time_point<std::chrono::steady_clock> m_IdleTimeout;
  std::map<std::string, std::list<std::string>> m_exportLists; // lower case host -> exports

  void mount(CNfsContext& context, const std::string& hostName);
  static std::string resolveHost(const CURL& url); //resolve hostname by dnslookup
  static void keepAlive(CNfsContext& context, struct nfsfh* _pFileHandle);
  // returns the NFS version the context will use
  static int setOptions(struct nfs_context* context);
};

extern CNfsConnection gNfsConnection;

namespace XFILE
{
  class CNFSFile : public IFile
  {
  public:
    CNFSFile();
    ~CNFSFile() override;
    void Close() override;
    int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
    ssize_t Read(void* lpBuf, size_t uiBufSize) override;
    bool Open(const CURL& url) override;
    bool Exists(const CURL& url) override;
    int Stat(const CURL& url, struct __stat64* buffer) override;
    int Stat(struct __stat64* buffer) override;
    int64_t GetLength() override;
    int64_t GetPosition() override;
    ssize_t Write(const void* lpBuf, size_t uiBufSize) override;
    int Truncate(int64_t iSize) override;

    //implement iocontrol for seek_possible for preventing the stat in File class for
    //getting this info ...
    int IoControl(IOControl request, void* param) override
    {
      return request == IOControl::SEEK_POSSIBLE ? 1 : -1;
    }
    int GetChunkSize() override;

    bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
    bool Delete(const CURL& url) override;
    bool Rename(const CURL& url, const CURL& urlnew) override;
  protected:
    CURL m_url;
    bool IsValidFile(const std::string& strFileName);
    int64_t m_fileSize = 0;
    struct nfsfh *m_pFileHandle;
    CNfsContextLease m_context;
  };
}

