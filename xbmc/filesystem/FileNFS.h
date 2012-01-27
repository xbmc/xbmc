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

// FileNFS.h: interface for the CFileNFS class.
#ifndef FILENFS_H_
#define FILENFS_H_

#include "IFile.h"
#include "URL.h"
#include "threads/CriticalSection.h"
#include <list>
#include "SectionLoader.h"
#include <map>

#ifdef TARGET_WINDOWS
#define S_IRGRP 0
#define S_IROTH 0
#define S_IWUSR _S_IWRITE
#define S_IRUSR _S_IREAD
#define	S_IFLNK 0120000

#define S_ISBLK(m) (0)
#define S_ISSOCK(m) (0)
#define S_ISLNK(m) ((m & S_IFLNK) != 0)
#define S_ISCHR(m) ((m & _S_IFCHR) != 0)
#define S_ISDIR(m) ((m & _S_IFDIR) != 0)
#define S_ISFIFO(m) ((m & _S_IFIFO) != 0)
#define S_ISREG(m) ((m & _S_IFREG) != 0)
#endif

class DllLibNfs;

class CNfsConnection : public CCriticalSection
{     
public:
  typedef std::map<struct nfsfh  *, unsigned int> tFileKeepAliveMap;  

  struct contextTimeout
  {
    struct nfs_context *pContext;
    uint64_t lastAccessedTime;
  };

  typedef std::map<std::string, struct contextTimeout> tOpenContextMap;    
  
  CNfsConnection();
  ~CNfsConnection();
  bool Connect(const CURL &url, CStdString &relativePath);
  struct nfs_context *GetNfsContext(){return m_pNfsContext;}
  size_t            GetMaxReadChunkSize(){return m_readChunkSize;}
  size_t            GetMaxWriteChunkSize(){return m_writeChunkSize;} 
  DllLibNfs        *GetImpl(){return m_pLibNfs;}
  std::list<CStdString> GetExportList(const CURL &url);
  //this functions splits the url into the exportpath (feed to mount) and the rest of the path
  //relative to the mounted export
  bool splitUrlIntoExportAndPath(const CURL& url, CStdString &exportPath, CStdString &relativePath);
  
  //special stat which uses its own context
  //needed for getting intervolume symlinks to work
  int stat(const CURL &url, struct stat *statbuff);

  void AddActiveConnection();
  void AddIdleConnection();
  void CheckIfIdle();
  void Deinit();
  bool HandleDyLoad();//loads the lib if needed
  //adds the filehandle to the keep alive list or resets
  //the timeout for this filehandle if already in list
  void resetKeepAlive(struct nfsfh  *_pFileHandle);
  //removes file handle from keep alive list
  void removeFromKeepAliveList(struct nfsfh  *_pFileHandle);  
  
  const CStdString& GetConnectedIp() const {return m_resolvedHostName;}
  const CStdString& GetConnectedExport() const {return m_exportPath;}

private:
  struct nfs_context *m_pNfsContext;//current nfs context
  CStdString m_exportPath;//current connected export path
  CStdString m_hostName;//current connected host
  CStdString m_resolvedHostName;//current connected host - as ip
  size_t m_readChunkSize;//current read chunksize of connected server
  size_t m_writeChunkSize;//current write chunksize of connected server
  int m_OpenConnections;//number of open connections
  unsigned int m_IdleTimeout;//timeout for idle connection close and dyunload
  tFileKeepAliveMap m_KeepAliveTimeouts;//mapping filehandles to its idle timeout
  tOpenContextMap m_openContextMap;//unique map for tracking all open contexts
  DllLibNfs *m_pLibNfs;//the lib
  std::list<CStdString> m_exportList;//list of exported pathes of current connected servers
  CCriticalSection keepAliveLock;
 
  void clearMembers();
  struct nfs_context *getContextFromMap(const CStdString &exportname);
  int  getContextForExport(const CStdString &exportname);//get context for given export and add to open contexts map - sets m_pNfsContext (my return a already mounted cached context)
  void destroyOpenContexts();
  void resolveHost(const CURL &url);//resolve hostname by dnslookup
  void keepAlive(struct nfsfh  *_pFileHandle);
};

extern CNfsConnection gNfsConnection;

namespace XFILE
{
  class CFileNFS : public IFile
  {
  public:
    CFileNFS();
    virtual ~CFileNFS();
    virtual void Close();
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int Stat(struct __stat64* buffer);
    virtual int64_t GetLength();
    virtual int64_t GetPosition();
    virtual int Write(const void* lpBuf, int64_t uiBufSize);
    //implement iocontrol for seek_possible for preventing the stat in File class for
    //getting this info ...
    virtual int IoControl(EIoControl request, void* param){ if(request == IOCTRL_SEEK_POSSIBLE) return 1;return -1;};    
    virtual int  GetChunkSize() {return 1;}
    
    virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& url, const CURL& urlnew);    
  protected:
    CURL m_url;
    bool IsValidFile(const CStdString& strFileName);
    int64_t m_fileSize;
    struct nfsfh  *m_pFileHandle;
    struct nfs_context *m_pNfsContext;//current nfs context    
  };
}
#endif // FILENFS_H_


