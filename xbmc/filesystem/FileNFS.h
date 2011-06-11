
// FileNFS.h: interface for the CFileNFS class.
#ifndef FILENFS_H_
#define FILENFS_H_

#include "IFile.h"
#include "URL.h"
#include "threads/CriticalSection.h"

#include "DllLibNfs.h"

CStdString URLEncode(CStdString str);

class CNfsConnection : public CCriticalSection
{     
public:
  
  CNfsConnection();
  ~CNfsConnection();
  bool Connect(const CURL &url);
  struct nfs_context *GetNfsContext(){return m_pNfsContext;}
  size_t            GetMaxReadChunkSize(){return m_readChunkSize;}
  size_t            GetMaxWriteChunkSize(){return m_writeChunkSize;} 
  DllLibNfs        *GetImpl(){return &m_libNfs;}
  
  void AddActiveConnection();
  void AddIdleConnection();
  void CheckIfIdle();
  void SetActivityTime();
  void Deinit();
  
private:
  struct nfs_context *m_pNfsContext;    
  CStdString m_shareName;
  CStdString m_hostName;
  size_t m_readChunkSize;
  size_t m_writeChunkSize;
  int m_OpenConnections;
  unsigned int m_IdleTimeout;
  DllLibNfs m_libNfs;
  void resetContext();  
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

