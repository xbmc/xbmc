/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// FileNFS.cpp: implementation of the CNFSFile class.
//
//////////////////////////////////////////////////////////////////////
#include "system.h"

#ifdef HAS_FILESYSTEM_NFS
#include "DllLibNfs.h"
#include "NFSFile.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "network/DNSNameCache.h"
#include "threads/SystemClock.h"

#include <nfsc/libnfs-raw-mount.h>

#ifdef TARGET_WINDOWS
#include <fcntl.h>
#include <sys\stat.h>
#endif

//KEEP_ALIVE_TIMEOUT is decremented every half a second
//480 * 0.5s == 240s == 4mins
//so when no read was done for 4mins and files are open
//do the nfs keep alive for the open files
#define KEEP_ALIVE_TIMEOUT 480

//4 mins cached context timeout
#define CONTEXT_TIMEOUT 240000

//return codes for getContextForExport
#define CONTEXT_INVALID  0    //getcontext failed
#define CONTEXT_NEW      1    //new context created
#define CONTEXT_CACHED   2    //context cached and therefore already mounted (no new mount needed)

using namespace XFILE;

CNfsConnection::CNfsConnection()
: m_pNfsContext(NULL)
, m_exportPath("")
, m_hostName("")
, m_resolvedHostName("")
, m_readChunkSize(0)
, m_writeChunkSize(0)
, m_OpenConnections(0)
, m_IdleTimeout(0)
, m_lastAccessedTime(0)
, m_pLibNfs(new DllLibNfs())
{
}

CNfsConnection::~CNfsConnection()
{
  Deinit();
  delete m_pLibNfs;
}

void CNfsConnection::resolveHost(const CURL &url)
{ 
  //resolve if hostname has changed
  CDNSNameCache::Lookup(url.GetHostName(), m_resolvedHostName);
}

std::list<CStdString> CNfsConnection::GetExportList(const CURL &url)
{
    std::list<CStdString> retList;

    if(HandleDyLoad())
    {
      struct exportnode *exportlist, *tmp;
      exportlist = m_pLibNfs->mount_getexports(m_resolvedHostName);
      tmp = exportlist;

      for(tmp = exportlist; tmp!=NULL; tmp=tmp->ex_next)
      {
        retList.push_back(CStdString(tmp->ex_dir));
      }      

      gNfsConnection.GetImpl()->mount_free_export_list(exportlist);
      retList.sort();
      retList.reverse();
    }
    
    return retList;
}

bool CNfsConnection::HandleDyLoad()
{
  bool ret = true;
  
  if(!m_pLibNfs->IsLoaded())
  {
    if(!m_pLibNfs->Load())
    {
      CLog::Log(LOGERROR,"NFS: Error loading libnfs (%s).",__FUNCTION__);    
      ret = false; //fatal
    }    
  }
  return ret;
}

void CNfsConnection::clearMembers()
{
    m_exportPath.clear();
    m_hostName.clear();
    m_exportList.clear();
    m_writeChunkSize = 0;
    m_readChunkSize = 0;  
    m_pNfsContext = NULL;
    m_KeepAliveTimeouts.clear();
}

void CNfsConnection::destroyOpenContexts()
{
  for(tOpenContextMap::iterator it = m_openContextMap.begin();it!=m_openContextMap.end();it++)
  {
    m_pLibNfs->nfs_destroy_context(it->second.pContext);
  }
  m_openContextMap.clear();
}

struct nfs_context *CNfsConnection::getContextFromMap(const CStdString &exportname)
{
  struct nfs_context *pRet = NULL;

  tOpenContextMap::iterator it = m_openContextMap.find(exportname.c_str());
  if(it != m_openContextMap.end())
  {
    //check if context has timed out already
    uint64_t now = XbmcThreads::SystemClockMillis();
    if((now - it->second.lastAccessedTime) < CONTEXT_TIMEOUT)
    {
      //its not timedout yet
      //refresh access time of that
      //context and return it
      CLog::Log(LOGDEBUG, "NFS: Refreshing context for %s, old: %"PRId64", new: %"PRId64, exportname.c_str(), it->second.lastAccessedTime, now);
      it->second.lastAccessedTime = now;
      pRet = it->second.pContext;
    }
    else 
    {
      //context is timed out
      //destroy it and return NULL
      CLog::Log(LOGDEBUG, "NFS: Old context timed out - destroying it");
      m_pLibNfs->nfs_destroy_context(it->second.pContext);
    }
  }
  return pRet;
}

int CNfsConnection::getContextForExport(const CStdString &exportname)
{
  int ret = CONTEXT_INVALID; 
    
  if(HandleDyLoad())
  {
    clearMembers();  
    
    m_pNfsContext = getContextFromMap(exportname);

    if(!m_pNfsContext)
    {
      CLog::Log(LOGDEBUG,"NFS: Context for %s not open - get a new context.", exportname.c_str());
      m_pNfsContext = m_pLibNfs->nfs_init_context();
    
      if(!m_pNfsContext) 
      {
        CLog::Log(LOGERROR,"NFS: Error initcontext in getContextForExport.");
      }
      else 
      {
        struct contextTimeout tmp;
        tmp.pContext = m_pNfsContext;
        tmp.lastAccessedTime = XbmcThreads::SystemClockMillis();
        m_openContextMap[exportname] = tmp; //add context to list of all contexts      
        ret = CONTEXT_NEW;
      }
    }
    else
    {
      ret = CONTEXT_CACHED;
      CLog::Log(LOGDEBUG,"NFS: Using cached context.");
    }
    m_lastAccessedTime = XbmcThreads::SystemClockMillis(); //refresh last access time of m_pNfsContext
  }
  return ret;
}

bool CNfsConnection::splitUrlIntoExportAndPath(const CURL& url, CStdString &exportPath, CStdString &relativePath)
{
    bool ret = false;
    
    //refresh exportlist if empty or hostname change
    if(m_exportList.empty() || !url.GetHostName().Equals(m_hostName,false))
    {
      m_exportList = GetExportList(url);
    }

    if(!m_exportList.empty())
    {
      relativePath = "";
      exportPath = "";
      
      CStdString path = url.GetFileName();
      
      //GetFileName returns path without leading "/"
      //but we need it because the export paths start with "/"
      //and path.Find(*it) wouldn't work else
      if(!path.empty() && path[0] != '/')
      {
        path = "/" + path;
      }
      
      std::list<CStdString>::iterator it;
      
      for(it=m_exportList.begin();it!=m_exportList.end();it++)
      {
        //if path starts with the current export path
        if( path.Find(*it) ==  0 )
        {
          exportPath = *it;
          //handle special case where root is exported
          //in that case we don't want to stripp off to
          //much from the path
          if( exportPath == "/" )
            relativePath = "//" + path.Right((path.length()) - exportPath.length());
          else
            relativePath = "//" + path.Right((path.length()-1) - exportPath.length());
          ret = true;
          break;          
        }
      }
    }
    return ret;
}

bool CNfsConnection::Connect(const CURL& url, CStdString &relativePath)
{
  CSingleLock lock(*this);
  bool ret = false;
  int nfsRet = 0;
  CStdString exportPath = "";

  resolveHost(url);
  ret = splitUrlIntoExportAndPath(url, exportPath, relativePath);
  
  if( (ret && (!exportPath.Equals(m_exportPath,true)  || 
      !url.GetHostName().Equals(m_hostName,false)))    ||
      (XbmcThreads::SystemClockMillis() - m_lastAccessedTime) > CONTEXT_TIMEOUT )
  {
    int contextRet = getContextForExport(url.GetHostName() + exportPath);
    
    if(contextRet == CONTEXT_INVALID)//we need a new context because sharename or hostname has changed
    {
      return false;
    }
    
    if(contextRet == CONTEXT_NEW) //new context was created - we need to mount it
    {
      //we connect to the directory of the path. This will be the "root" path of this connection then.
      //So all fileoperations are relative to this mountpoint...
      nfsRet = m_pLibNfs->nfs_mount(m_pNfsContext, m_resolvedHostName.c_str(), exportPath.c_str());

      if(nfsRet != 0) 
      {
        CLog::Log(LOGERROR,"NFS: Failed to mount nfs share: %s (%s)\n", exportPath.c_str(), m_pLibNfs->nfs_get_error(m_pNfsContext));
        return false;
      }
      CLog::Log(LOGDEBUG,"NFS: Connected to server %s and export %s\n", url.GetHostName().c_str(), exportPath.c_str());
    }
    m_exportPath = exportPath;
    m_hostName = url.GetHostName();
    //read chunksize only works after mount
    m_readChunkSize = m_pLibNfs->nfs_get_readmax(m_pNfsContext);
    m_writeChunkSize = m_pLibNfs->nfs_get_writemax(m_pNfsContext);

    if(contextRet == CONTEXT_NEW)
    {
      CLog::Log(LOGDEBUG,"NFS: chunks: r/w %i/%i\n", (int)m_readChunkSize,(int)m_writeChunkSize);          
    }
  }
  return ret; 
}

void CNfsConnection::Deinit()
{
  if(m_pNfsContext && m_pLibNfs->IsLoaded())
  {
    destroyOpenContexts();
    m_pNfsContext = NULL;
    m_pLibNfs->Unload();    
  }        
  clearMembers();
}

/* This is called from CApplication::ProcessSlow() and is used to tell if nfs have been idle for too long */
void CNfsConnection::CheckIfIdle()
{
  /* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0 && m_pNfsContext != NULL)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    CSingleLock lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */)
    {
      if (m_IdleTimeout > 0)
      {
        m_IdleTimeout--;
      }
      else
      {
        CLog::Log(LOGNOTICE, "NFS is idle. Closing the remaining connections.");
        gNfsConnection.Deinit();
      }
    }
  }
  
  if( m_pNfsContext != NULL )
  {
    //handle keep alive on opened files
    for( tFileKeepAliveMap::iterator it = m_KeepAliveTimeouts.begin();it!=m_KeepAliveTimeouts.end();it++)
    {
      CSingleLock lock(keepAliveLock);
      if(it->second > 0)
      {
        it->second--;
      }
      else
      {
        lock.Leave();
        keepAlive(it->first);
        //reset timeout
        resetKeepAlive(it->first);
      }
    }
  }
}

//remove file handle from keep alive list on file close
void CNfsConnection::removeFromKeepAliveList(struct nfsfh  *_pFileHandle)
{
  CSingleLock lock(keepAliveLock);
  m_KeepAliveTimeouts.erase(_pFileHandle);
}

//reset timeouts on read
void CNfsConnection::resetKeepAlive(struct nfsfh  *_pFileHandle)
{
  CSingleLock lock(keepAliveLock);
  //adds new keys - refreshs existing ones  
  m_KeepAliveTimeouts[_pFileHandle] = KEEP_ALIVE_TIMEOUT;
}

//keep alive the filehandles nfs connection
//by blindly doing a read 32bytes - seek back to where
//we were before
void CNfsConnection::keepAlive(struct nfsfh  *_pFileHandle)
{
  uint64_t offset = 0;
  char buffer[32];
  CLog::Log(LOGNOTICE, "NFS: sending keep alive after %i s.",KEEP_ALIVE_TIMEOUT/2);
  CSingleLock lock(*this);
  m_pLibNfs->nfs_lseek(m_pNfsContext, _pFileHandle, 0, SEEK_CUR, &offset);
  m_pLibNfs->nfs_read(m_pNfsContext, _pFileHandle, 32, buffer);
  m_pLibNfs->nfs_lseek(m_pNfsContext, _pFileHandle, offset, SEEK_SET, &offset);
}

int CNfsConnection::stat(const CURL &url, struct stat *statbuff)
{
  CSingleLock lock(*this);
  int nfsRet = 0;
  CStdString exportPath;
  CStdString relativePath;
  struct nfs_context *pTmpContext = NULL;
  
  if(!HandleDyLoad())
  {
    return -1;
  }
  
  resolveHost(url);
  
  if(splitUrlIntoExportAndPath(url, exportPath, relativePath))
  {    
    pTmpContext = m_pLibNfs->nfs_init_context();
    
    if(pTmpContext)
    {  
      //we connect to the directory of the path. This will be the "root" path of this connection then.
      //So all fileoperations are relative to this mountpoint...
      nfsRet = m_pLibNfs->nfs_mount(pTmpContext, m_resolvedHostName.c_str(), exportPath.c_str());
      
      if(nfsRet == 0) 
      {
        nfsRet = m_pLibNfs->nfs_stat(pTmpContext, relativePath.c_str(), statbuff);      
      }
      else
      {
        CLog::Log(LOGERROR,"NFS: Failed to mount nfs share: %s (%s)\n", exportPath.c_str(), m_pLibNfs->nfs_get_error(m_pNfsContext));
      }
      
      m_pLibNfs->nfs_destroy_context(pTmpContext);
      CLog::Log(LOGDEBUG,"NFS: Connected to server %s and export %s in tmpContext\n", url.GetHostName().c_str(), exportPath.c_str());
    }
  }
  return nfsRet;
}

/* The following two function is used to keep track on how many Opened files/directories there are.
needed for unloading the dylib*/
void CNfsConnection::AddActiveConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections++;
}

void CNfsConnection::AddIdleConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
   leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

CNfsConnection gNfsConnection;

CNFSFile::CNFSFile()
: m_fileSize(0)
, m_pFileHandle(NULL)
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
  CSingleLock lock(gNfsConnection);
  
  if (gNfsConnection.GetNfsContext() == NULL || m_pFileHandle == NULL) return 0;
  
  ret = (int)gNfsConnection.GetImpl()->nfs_lseek(gNfsConnection.GetNfsContext(), m_pFileHandle, 0, SEEK_CUR, &offset);
  
  if (ret < 0) 
  {
    CLog::Log(LOGERROR, "NFS: Failed to lseek(%s)",gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
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
  int ret = 0;
  Close();
  // we can't open files like nfs://file.f or nfs://server/file.f
  // if a file matches the if below return false, it can't exist on a nfs share.
  if (!IsValidFile(url.GetFileName()))
  {
    CLog::Log(LOGNOTICE,"NFS: Bad URL : '%s'",url.GetFileName().c_str());
    return false;
  }
  
  CStdString filename = "";
   
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url, filename))
    return false;
  
  m_pNfsContext = gNfsConnection.GetNfsContext(); 
  
  ret = gNfsConnection.GetImpl()->nfs_open(m_pNfsContext, filename.c_str(), O_RDONLY, &m_pFileHandle);
  
  if (ret != 0) 
  {
    CLog::Log(LOGINFO, "CNFSFile::Open: Unable to open file : '%s'  error : '%s'", url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));
    return false;
  } 
  
  CLog::Log(LOGDEBUG,"CNFSFile::Open - opened %s",url.GetFileName().c_str());
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
  CSingleLock lock(gNfsConnection);
  CStdString filename = "";
  
  if(!gNfsConnection.Connect(url,filename))
    return -1;
   

  struct stat tmpBuffer = {0};

  ret = gNfsConnection.GetImpl()->nfs_stat(gNfsConnection.GetNfsContext(), filename.c_str(), &tmpBuffer);
  
  //if buffer == NULL we where called from Exists - in that case don't spam the log with errors
  if (ret != 0 && buffer != NULL) 
  {
    CLog::Log(LOGERROR, "NFS: Failed to stat(%s) %s\n", url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    ret = -1;
  }
  else
  {  
    if(buffer)
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_dev = tmpBuffer.st_dev;
      buffer->st_ino = tmpBuffer.st_ino;
      buffer->st_mode = tmpBuffer.st_mode;
      buffer->st_nlink = tmpBuffer.st_nlink;
      buffer->st_uid = tmpBuffer.st_uid;
      buffer->st_gid = tmpBuffer.st_gid;
      buffer->st_rdev = tmpBuffer.st_rdev;
      buffer->st_size = tmpBuffer.st_size;
      buffer->st_atime = tmpBuffer.st_atime;
      buffer->st_mtime = tmpBuffer.st_mtime;
      buffer->st_ctime = tmpBuffer.st_ctime;
    }
  }
  return ret;
}

unsigned int CNFSFile::Read(void *lpBuf, int64_t uiBufSize)
{
  int numberOfBytesRead = 0;
  CSingleLock lock(gNfsConnection);
  
  if (m_pFileHandle == NULL || m_pNfsContext == NULL ) return 0;

  numberOfBytesRead = gNfsConnection.GetImpl()->nfs_read(m_pNfsContext, m_pFileHandle, uiBufSize, (char *)lpBuf);  

  lock.Leave();//no need to keep the connection lock after that
  
  gNfsConnection.resetKeepAlive(m_pFileHandle);//triggers keep alive timer reset for this filehandle
  
  //something went wrong ...
  if (numberOfBytesRead < 0) 
  {
    CLog::Log(LOGERROR, "%s - Error( %d, %s )", __FUNCTION__, numberOfBytesRead, gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));
    return 0;
  }
  return (unsigned int)numberOfBytesRead;
}

int64_t CNFSFile::Seek(int64_t iFilePosition, int iWhence)
{
  int ret = 0;
  uint64_t offset = 0;

  CSingleLock lock(gNfsConnection);  
  if (m_pFileHandle == NULL || m_pNfsContext == NULL) return -1;
  
 
  ret = (int)gNfsConnection.GetImpl()->nfs_lseek(m_pNfsContext, m_pFileHandle, iFilePosition, iWhence, &offset);
  if (ret < 0) 
  {
    CLog::Log(LOGERROR, "%s - Error( seekpos: %"PRId64", whence: %i, fsize: %"PRId64", %s)", __FUNCTION__, iFilePosition, iWhence, m_fileSize, gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));
    return -1;
  }
  return (int64_t)offset;
}

int CNFSFile::Truncate(int64_t iSize)
{
  int ret = 0;
  
  CSingleLock lock(gNfsConnection);  
  if (m_pFileHandle == NULL || m_pNfsContext == NULL) return -1;
  
  
  ret = (int)gNfsConnection.GetImpl()->nfs_ftruncate(m_pNfsContext, m_pFileHandle, iSize);
  if (ret < 0) 
  {
    CLog::Log(LOGERROR, "%s - Error( ftruncate: %"PRId64", fsize: %"PRId64", %s)", __FUNCTION__, iSize, m_fileSize, gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));
    return -1;
  }
  return ret;
}

void CNFSFile::Close()
{
  CSingleLock lock(gNfsConnection);
  
  if (m_pFileHandle != NULL && m_pNfsContext != NULL)
  {
    int ret = 0;
    CLog::Log(LOGDEBUG,"CNFSFile::Close closing file %s", m_url.GetFileName().c_str());
    ret = gNfsConnection.GetImpl()->nfs_close(m_pNfsContext, m_pFileHandle);
    gNfsConnection.removeFromKeepAliveList(m_pFileHandle);
        
	  if (ret < 0) 
    {
      CLog::Log(LOGERROR, "Failed to close(%s) - %s\n", m_url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));
    }
    m_pFileHandle = NULL;
    m_pNfsContext = NULL;    
    m_fileSize = 0;
  }
}

//this was a bitch!
//for nfs write to work we have to write chunked
//otherwise this could crash on big files
int CNFSFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  int numberOfBytesWritten = 0;
  int writtenBytes = 0;
  int64_t leftBytes = uiBufSize;
  //clamp max write chunksize to 32kb - fixme - this might be superfluious with future libnfs versions
  int64_t chunkSize = gNfsConnection.GetMaxWriteChunkSize() > 32768 ? 32768 : gNfsConnection.GetMaxWriteChunkSize();
  
  CSingleLock lock(gNfsConnection);
  
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
    writtenBytes = gNfsConnection.GetImpl()->nfs_write(m_pNfsContext,
                                  m_pFileHandle, 
                                  chunkSize, 
                                  (char *)lpBuf + numberOfBytesWritten);
    //decrease left bytes
    leftBytes-= writtenBytes;
    //increase overall written bytes
    numberOfBytesWritten += writtenBytes;
        
    //danger - something went wrong
    if (writtenBytes < 0) 
    {
      CLog::Log(LOGERROR, "Failed to pwrite(%s) %s\n", m_url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(m_pNfsContext));        
      break;
    }     
  }
  //return total number of written bytes
  return numberOfBytesWritten;
}

bool CNFSFile::Delete(const CURL& url)
{
  int ret = 0;
  CSingleLock lock(gNfsConnection);
  CStdString filename = "";
  
  if(!gNfsConnection.Connect(url, filename))
    return false;
  
  
  ret = gNfsConnection.GetImpl()->nfs_unlink(gNfsConnection.GetNfsContext(), filename.c_str());
  
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  }
  return (ret == 0);
}

bool CNFSFile::Rename(const CURL& url, const CURL& urlnew)
{
  int ret = 0;
  CSingleLock lock(gNfsConnection);
  CStdString strFile = "";
  
  if(!gNfsConnection.Connect(url,strFile))
    return false;
  
  CStdString strFileNew;
  CStdString strDummy;
  gNfsConnection.splitUrlIntoExportAndPath(urlnew, strDummy, strFileNew);
  
  ret = gNfsConnection.GetImpl()->nfs_rename(gNfsConnection.GetNfsContext() , strFile.c_str(), strFileNew.c_str());
  
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
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
  CSingleLock lock(gNfsConnection);
  CStdString filename = "";
  
  if(!gNfsConnection.Connect(url,filename))
    return false;
  
  m_pNfsContext = gNfsConnection.GetNfsContext();
  
  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileNFS::OpenForWrite() called with overwriting enabled! - %s", filename.c_str());
    //create file with proper permissions
    ret = gNfsConnection.GetImpl()->nfs_creat(m_pNfsContext, filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &m_pFileHandle);    
    //if file was created the file handle isn't valid ... so close it and open later
    if(ret == 0)
    {
      gNfsConnection.GetImpl()->nfs_close(m_pNfsContext,m_pFileHandle);
      m_pFileHandle = NULL;          
    }
  }

  ret = gNfsConnection.GetImpl()->nfs_open(m_pNfsContext, filename.c_str(), O_RDWR, &m_pFileHandle);
  
  if (ret || m_pFileHandle == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "CNFSFile::Open: Unable to open file : '%s' error : '%s'", filename.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  m_url=url;
  
  struct __stat64 tmpBuffer = {0};

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

bool CNFSFile::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == -1 || /* doesn't have sharename */
      strFileName.Right(2) == "/." || /* not current folder */
      strFileName.Right(3) == "/..")  /* not parent folder */
    return false;
  return true;
}
#endif//HAS_FILESYSTEM_NFS

