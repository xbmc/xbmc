/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// FileNFS.cpp: implementation of the CFileNFS class.
//
//////////////////////////////////////////////////////////////////////
#include "system.h"

#ifdef HAS_FILESYSTEM_NFS
#include "DllLibNfs.h"
#include "FileNFS.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CNfsConnection::CNfsConnection()
: m_pNfsContext(NULL)
, m_shareName("")
, m_readChunkSize(0)
, m_writeChunkSize(0)
, m_OpenConnections(0)
, m_IdleTimeout(0)
, m_pLibNfs(new DllLibNfs())
{
}

CNfsConnection::~CNfsConnection()
{
  delete m_pLibNfs;
}

void CNfsConnection::resetContext()
{
  
  if(!m_pLibNfs->IsLoaded())
  {
    if(!m_pLibNfs->Load())
    {
      CLog::Log(LOGERROR,"NFS: Error loading libnfs (%s).",__FUNCTION__);    
      return;//FATAL!
    }    
  }
  
  if(m_pNfsContext)
  {
    m_pLibNfs->nfs_destroy_context(m_pNfsContext);
  }
  
  m_pNfsContext = m_pLibNfs->nfs_init_context();
  
  if (!m_pNfsContext) 
  {
		CLog::Log(LOGERROR,"NFS: Error initcontext in resetContext.");
  }
  m_writeChunkSize = 0;
  m_readChunkSize = 0;
  m_shareName.clear();
  m_hostName.clear();
}

bool CNfsConnection::Connect(const CURL& url)
{
  CSingleLock lock(*this);
  int ret = 0;
  CStdString share; 
  URIUtils::GetDirectory(url.GetFileName(),share);
  share = "/" + share;

  if(!share.Equals(m_shareName,true) || !url.GetHostName().Equals(m_hostName,false) )
  {
    resetContext();//we need a new context because sharename or hostname has changed - old context will be freed
    
    //we connect to the directory of the path. This will be the "root" path of this connection then.
    //So all fileoperations are relative to this mountpoint...
    ret = m_pLibNfs->nfs_mount_sync(m_pNfsContext, url.GetHostName().c_str(), share.c_str());

    if  (ret != 0) 
    {
      CLog::Log(LOGERROR,"NFS: Failed to mount nfs share: %s\n", m_pLibNfs->nfs_get_error(m_pNfsContext));
      return false;
    }
    m_shareName = share;
    m_hostName = url.GetHostName();
    m_readChunkSize = m_pLibNfs->nfs_get_readmax(m_pNfsContext);
    m_writeChunkSize = m_pLibNfs->nfs_get_writemax(m_pNfsContext);   
    CLog::Log(LOGDEBUG,"NFS: Connected to server %s and export %s (chunks: r/w %i/%i)\n", url.GetHostName().c_str(), url.GetShareName().c_str(),(int)m_readChunkSize,(int)m_writeChunkSize);
  }

  return true; 
}

void CNfsConnection::Deinit()
{
  if(m_pNfsContext)
  {
    m_pLibNfs->nfs_destroy_context(m_pNfsContext);
  }        
  m_pNfsContext = NULL;
  m_shareName.clear();
  m_hostName.clear();
  m_pLibNfs->Unload();
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
}

void CNfsConnection::SetActivityTime()
{
  /* Since we get called every 500ms from ProcessSlow we limit the tick count to 180 */
  /* That means we have 2 ticks per second which equals 180/2 == 90 seconds */
  m_IdleTimeout = 180;
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

CFileNFS::CFileNFS()
: m_fileSize(0)
, m_pFileHandle(NULL)
{
  gNfsConnection.AddActiveConnection();
}

CFileNFS::~CFileNFS()
{
  Close();
  gNfsConnection.AddIdleConnection();
}

int64_t CFileNFS::GetPosition()
{
  int ret = 0;
  off_t offset = 0;
  CSingleLock lock(gNfsConnection);
  
  if (gNfsConnection.GetNfsContext() == NULL || m_pFileHandle == NULL) return 0;
  
  ret = (int)gNfsConnection.GetImpl()->nfs_lseek_sync(gNfsConnection.GetNfsContext(), m_pFileHandle, 0, SEEK_CUR, &offset);
  
  if (ret < 0) 
  {
    CLog::Log(LOGERROR, "NFS: Failed to lseek(%s)",gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  }
  return offset;
}

int64_t CFileNFS::GetLength()
{
  if (m_pFileHandle == NULL) return 0;
  return m_fileSize;
}

bool CFileNFS::Open(const CURL& url)
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
  
  CStdString filename = "//" + URIUtils::GetFileName(url.GetFileName());
   
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  ret = gNfsConnection.GetImpl()->nfs_open_sync(gNfsConnection.GetNfsContext(), filename.c_str(), O_RDONLY, &m_pFileHandle);
  
  if (ret != 0) 
  {
    CLog::Log(LOGINFO, "CFileNFS::Open: Unable to open file : '%s'  error : '%s'", url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  } 
  
  CLog::Log(LOGDEBUG,"CFileNFS::Open - opened %s",url.GetFileName().c_str());
  m_url=url;
  
#ifdef _LINUX
  struct __stat64 tmpBuffer;
#else
  struct stat tmpBuffer;
#endif
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


bool CFileNFS::Exists(const CURL& url)
{
  return Stat(url,NULL) == 0;
}

int CFileNFS::Stat(struct __stat64* buffer)
{
  return Stat(m_url,buffer);
}


int CFileNFS::Stat(const CURL& url, struct __stat64* buffer)
{
  int ret = 0;
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url))
    return -1;
   
  CStdString filename = "//" + URIUtils::GetFileName(url.GetFileName());

  struct stat tmpBuffer = {0};

  ret = gNfsConnection.GetImpl()->nfs_stat_sync(gNfsConnection.GetNfsContext(), filename.c_str(), &tmpBuffer);
  
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

unsigned int CFileNFS::Read(void *lpBuf, int64_t uiBufSize)
{
  int numberOfBytesRead = 0;
  int bytesLeft = uiBufSize;
  int bytesRead = 0;
  int chunkSize = gNfsConnection.GetMaxReadChunkSize();
  CSingleLock lock(gNfsConnection);
  
  if (m_pFileHandle == NULL || gNfsConnection.GetNfsContext()==NULL ) return 0;

  //read chunked since nfs will only give 16kb at once
  while(bytesLeft)
  {
    //last chunk could be smaller then chunk size
    if(bytesLeft < chunkSize)
    {
      chunkSize = bytesLeft; 
    }
    
    bytesRead = gNfsConnection.GetImpl()->nfs_read_sync(gNfsConnection.GetNfsContext(), m_pFileHandle, chunkSize, (char *)lpBuf+numberOfBytesRead);
    bytesLeft -= bytesRead;
    numberOfBytesRead += bytesRead;
    
    if(bytesRead == 0)
    {
      break; //EOF
    }
    
    //something went wrong ...
    if (bytesRead < 0) 
    {
      CLog::Log(LOGERROR, "%s - Error( %d, %s )", __FUNCTION__, bytesRead, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
      return 0;
    }
    
  }
  return (unsigned int)numberOfBytesRead;
}

int64_t CFileNFS::Seek(int64_t iFilePosition, int iWhence)
{
  int ret = 0;
  off_t offset = 0;

  CSingleLock lock(gNfsConnection);  
  if (m_pFileHandle == NULL || gNfsConnection.GetNfsContext()==NULL) return -1;
  
 
  ret = (int)gNfsConnection.GetImpl()->nfs_lseek_sync(gNfsConnection.GetNfsContext(), m_pFileHandle, iFilePosition, iWhence, &offset);
  
  if (ret < 0) 
  {
    CLog::Log(LOGERROR, "%s - Error( seekpos: %"PRId64", whence: %i, fsize: %"PRId64", %s)", __FUNCTION__, iFilePosition, iWhence, m_fileSize, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return -1;
  }
  return (int64_t)offset;
}

void CFileNFS::Close()
{
  CSingleLock lock(gNfsConnection);
  
  if (m_pFileHandle != NULL && gNfsConnection.GetNfsContext()!=NULL)
  {
    int ret = 0;
    CLog::Log(LOGDEBUG,"CFileNFS::Close closing file %s", m_url.GetFileName().c_str());
    ret = gNfsConnection.GetImpl()->nfs_close_sync(gNfsConnection.GetNfsContext(), m_pFileHandle);
    
	  if (ret < 0) 
    {
      CLog::Log(LOGERROR, "Failed to close(%s) - %s\n", m_url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    }
    m_pFileHandle=NULL;
    m_fileSize = 0;
  }
}

//this was a bitch!
//for nfs write to work we have to write chunked
//otherwise this could crash on big files
int CFileNFS::Write(const void* lpBuf, int64_t uiBufSize)
{
  int numberOfBytesWritten = 0;
  int writtenBytes = 0;
  int leftBytes = uiBufSize;
  int chunkSize = gNfsConnection.GetMaxWriteChunkSize();
  
  CSingleLock lock(gNfsConnection);
  
  if (m_pFileHandle == NULL || gNfsConnection.GetNfsContext() == NULL) return -1;
  
  //write as long as some bytes are left to be written
  while( leftBytes )
  {
    //the last chunk could be smalle than chunksize
    if(leftBytes < chunkSize)
    {
      chunkSize = leftBytes;//write last chunk with correct size
    }
    //write chunk
    writtenBytes = gNfsConnection.GetImpl()->nfs_write_sync(gNfsConnection.GetNfsContext(), 
                                  m_pFileHandle, 
                                  (size_t)chunkSize, 
                                  (char *)lpBuf + numberOfBytesWritten);
    //decrease left bytes
    leftBytes-= writtenBytes;
    //increase overall written bytes
    numberOfBytesWritten += writtenBytes;
        
    //danger - something went wrong
    if (writtenBytes < 0) 
    {
      CLog::Log(LOGERROR, "Failed to pwrite(%s) %s\n", m_url.GetFileName().c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));        
      break;
    }     
  }
  //return total number of written bytes
  return numberOfBytesWritten;
}

bool CFileNFS::Delete(const CURL& url)
{
  int ret = 0;
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  CStdString filename = "//" + URIUtils::GetFileName(url.GetFileName());
  
  ret = gNfsConnection.GetImpl()->nfs_unlink_sync(gNfsConnection.GetNfsContext(), filename.c_str());
  
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  }
  return (ret == 0);
}

bool CFileNFS::Rename(const CURL& url, const CURL& urlnew)
{
  int ret = 0;
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  CStdString strFile = "//" + URIUtils::GetFileName(url.GetFileName());
  CStdString strFileNew = "//" + URIUtils::GetFileName(urlnew.GetFileName());
  
  ret = gNfsConnection.GetImpl()->nfs_rename_sync(gNfsConnection.GetNfsContext() , strFile.c_str(), strFileNew.c_str());
  
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  } 
  return (ret == 0);
}

bool CFileNFS::OpenForWrite(const CURL& url, bool bOverWrite)
{ 
  int ret = 0;
  
  Close();
  CSingleLock lock(gNfsConnection);
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  // we can't open files like nfs://file.f or nfs://server/file.f
  // if a file matches the if below return false, it can't exist on a nfs share.
  if (!IsValidFile(url.GetFileName())) return false;
  
  CStdString filename = "//" + URIUtils::GetFileName(url.GetFileName());
  
  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileNFS::OpenForWrite() called with overwriting enabled! - %s", filename.c_str());
    //create file with proper permissions
    ret = gNfsConnection.GetImpl()->nfs_creat_sync(gNfsConnection.GetNfsContext(), filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &m_pFileHandle);    
    //if file was created the file handle isn't valid ... so close it and open later
    if(ret == 0)
    {
      gNfsConnection.GetImpl()->nfs_close_sync(gNfsConnection.GetNfsContext(),m_pFileHandle);
    }
  }

  ret = gNfsConnection.GetImpl()->nfs_open_sync(gNfsConnection.GetNfsContext(), filename.c_str(), O_RDWR, &m_pFileHandle);
  
  if (ret || m_pFileHandle == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "CFileNFS::Open: Unable to open file : '%s' error : '%s'", filename.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  m_url=url;
  
#ifdef _LINUX
  struct __stat64 tmpBuffer = {0};
#else
  struct stat tmpBuffer = {0};
#endif
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

bool CFileNFS::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == -1 || /* doesn't have sharename */
      strFileName.Right(2) == "/." || /* not current folder */
      strFileName.Right(3) == "/..")  /* not parent folder */
    return false;
  return true;
}
#endif//HAS_FILESYSTEM_NFS
