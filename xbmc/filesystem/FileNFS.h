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

class DllLibNfs;

class CNfsConnection : public CCriticalSection
{     
public:
  
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

  void AddActiveConnection();
  void AddIdleConnection();
  void CheckIfIdle();
  void SetActivityTime();
  void Deinit();
  bool HandleDyLoad();//loads the lib if needed

private:
  struct nfs_context *m_pNfsContext;//current nfs context
  CStdString m_exportPath;//current connected export path
  CStdString m_hostName;//current connected host
  CStdString m_resolvedHostName;//current connected host - as ip
  size_t m_readChunkSize;//current read chunksize of connected server
  size_t m_writeChunkSize;//current write chunksize of connected server
  int m_OpenConnections;//number of open connections
  unsigned int m_IdleTimeout;//timeout for idle connection close and dyunload
  DllLibNfs *m_pLibNfs;//the lib
  std::list<CStdString> m_exportList;//list of exported pathes of current connected servers
  
  void clearMembers();
  bool resetContext();//clear old nfs context and init new context
  void resolveHost(const CURL &url);//resolve hostname by dnslookup
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
  };
}
#endif // FILENFS_H_

