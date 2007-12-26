#ifdef HAS_MMS

#ifndef FILEMMS_H_
#define FILEMMS_H_

#include "IFile.h"
#include "libmms/mms.h"

namespace XFILE
{
class CFileMMS : public IFile
{
public:
  CFileMMS();
  virtual ~CFileMMS();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual CStdString GetContent();

protected:
  mms_t* m_mms;
  CStdString m_contenttype;
};
};
#endif // !defined(AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_)

#endif
