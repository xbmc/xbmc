// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ifile.h"
using namespace XFILE;

namespace XFILE
{
class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent) = 0;
};
class CFile
{
public:
  CFile();
  virtual ~CFile();

  bool Open(const CStdString& strFileName, bool bBinary = true);
  bool OpenForWrite(const CStdString& strFileName, bool bBinary = true, bool bOverWrite = false);
  unsigned int Read(void* lpBuf, __int64 uiBufSize);
  bool ReadString(char *szLine, int iLineLength);
  int Write(const void* lpBuf, __int64 uiBufSize);
  void Flush();
  __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  __int64 GetPosition();
  __int64 GetLength();
  void Close();


  static bool Exists(const CStdString& strFileName);
  static int  Stat(const CStdString& strFileName, struct __stat64* buffer);
  static bool Delete(const CStdString& strFileName);
  static bool Rename(const CStdString& strFileName, const CStdString& strNewFileName);
  static bool Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);

private:
  IFile* m_pFile;
};
};
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
