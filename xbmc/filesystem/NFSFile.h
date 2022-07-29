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

#include <chrono>
#include <list>
#include <map>

struct nfs_stat_64;

class CNfsConnection : public CCriticalSection
{
public:
  struct keepAliveStruct
  {
    std::string exportPath;
    std::chrono::time_point<std::chrono::steady_clock> refreshTime;
  };
  typedef std::map<struct nfsfh  *, struct keepAliveStruct> tFileKeepAliveMap;

  struct contextTimeout
  {
    struct nfs_context *pContext;
    std::chrono::time_point<std::chrono::steady_clock> lastAccessedTime;
  };

  typedef std::map<std::string, struct contextTimeout> tOpenContextMap;

  CNfsConnection();
  ~CNfsConnection();
  bool Connect(const CURL &url, std::string &relativePath);
  struct nfs_context *GetNfsContext() {return m_pNfsContext;}
  uint64_t GetMaxReadChunkSize() {return m_readChunkSize;}
  uint64_t GetMaxWriteChunkSize() {return m_writeChunkSize;}
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
  //adds the filehandle to the keep alive list or resets
  //the timeout for this filehandle if already in list
  void resetKeepAlive(const std::string& _exportPath, struct nfsfh* _pFileHandle);
  //removes file handle from keep alive list
  void removeFromKeepAliveList(struct nfsfh  *_pFileHandle);

  const std::string& GetConnectedIp() const {return m_resolvedHostName;}
  const std::string& GetConnectedExport() const {return m_exportPath;}
  const std::string GetContextMapId() const {return m_hostName + m_exportPath;}

private:
  enum class ContextStatus
  {
    INVALID,
    NEW,
    CACHED
  };

  struct nfs_context *m_pNfsContext;//current nfs context
  std::string m_exportPath;//current connected export path
  std::string m_hostName;//current connected host
  std::string m_resolvedHostName;//current connected host - as ip
  uint64_t m_readChunkSize = 0;//current read chunksize of connected server
  uint64_t m_writeChunkSize = 0;//current write chunksize of connected server
  int m_OpenConnections = 0; //number of open connections
  std::chrono::time_point<std::chrono::steady_clock> m_IdleTimeout;
  tFileKeepAliveMap m_KeepAliveTimeouts;//mapping filehandles to its idle timeout
  tOpenContextMap m_openContextMap;//unique map for tracking all open contexts
  std::chrono::time_point<std::chrono::steady_clock>
      m_lastAccessedTime; //last access time for m_pNfsContext
  std::list<std::string> m_exportList;//list of exported paths of current connected servers
  CCriticalSection keepAliveLock;
  CCriticalSection openContextLock;

  void clearMembers();
  struct nfs_context *getContextFromMap(const std::string &exportname, bool forceCacheHit = false);

  // get context for given export and add to open contexts map - sets m_pNfsContext (may return an already mounted cached context)
  ContextStatus getContextForExport(const std::string& exportname);
  void destroyOpenContexts();
  void destroyContext(const std::string &exportName);
  void resolveHost(const CURL &url);//resolve hostname by dnslookup
  void keepAlive(const std::string& _exportPath, struct nfsfh* _pFileHandle);
  static void setOptions(struct nfs_context* context);
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
    int IoControl(EIoControl request, void* param) override
    {
      return request == IOCTRL_SEEK_POSSIBLE ? 1 : -1;
    }
    int GetChunkSize() override {return static_cast<int>(gNfsConnection.GetMaxReadChunkSize());}

    bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
    bool Delete(const CURL& url) override;
    bool Rename(const CURL& url, const CURL& urlnew) override;
  protected:
    CURL m_url;
    bool IsValidFile(const std::string& strFileName);
    int64_t m_fileSize = 0;
    struct nfsfh *m_pFileHandle;
    struct nfs_context *m_pNfsContext;//current nfs context
    std::string m_exportPath;
  };
}

