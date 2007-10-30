// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
#define AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

namespace XFILE
{
class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) = 0;
  virtual ~IFileCallback() {}; 
};

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01

/* use buffered io during reading, ( hint to make all protocols buffered, some might be internal anyway ) */
#define READ_BUFFERED  0x02

class CFileStreamBuffer;

class CFile
{
public:
  CFile();
  virtual ~CFile();

  bool Open(const CStdString& strFileName, bool bBinary = true, unsigned int flags = 0);
  bool OpenForWrite(const CStdString& strFileName, bool bBinary = true, bool bOverWrite = false);  
  unsigned int Read(void* lpBuf, __int64 uiBufSize);
  bool ReadString(char *szLine, int iLineLength);
  int Write(const void* lpBuf, __int64 uiBufSize);
  void Flush();
  __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  __int64 GetPosition();
  __int64 GetLength();
  void Close();
  int GetChunkSize() {if (m_pFile) return m_pFile->GetChunkSize(); return 0;}
  bool SkipNext(){if (m_pFile) return m_pFile->SkipNext(); return false;}
  void SetObject(void* obj){if (m_pFile) m_pFile->Object = obj;} //generic object pointer to whatever


  static bool Exists(const CStdString& strFileName);
  static int  Stat(const CStdString& strFileName, struct __stat64* buffer);
  static bool Delete(const CStdString& strFileName);
  static bool Rename(const CStdString& strFileName, const CStdString& strNewFileName);
  static bool Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);

private:
  unsigned int m_flags;
  IFile* m_pFile;
  CFileStreamBuffer* m_pBuffer;
};

// streambuf for file io, only supports buffered input currently
class CFileStreamBuffer
  : public std::streambuf
{
public:
  ~CFileStreamBuffer();
  CFileStreamBuffer(int backsize = 0);
  
  void Attach(IFile *file);
  void Detach();

private:
  virtual int_type underflow();
  virtual streamsize showmanyc();
	virtual pos_type seekoff(off_type, ios_base::seekdir,	ios_base::openmode = ios_base::in | ios_base::out);
	virtual pos_type seekpos(pos_type, ios_base::openmode = ios_base::in | ios_base::out);

  IFile* m_file;
  char*  m_buffer;
  int    m_backsize;
  int    m_frontsize;
};

// very basic file input stream
class CFileStream
  : public std::istream
{
public:
  CFileStream(int backsize = 0);
  ~CFileStream();

  bool Open(const CStdString& filename)  { return Open(CURL(filename)); }
  bool Open(const CURL& filename);
  void Close();

  __int64 GetLength();
private:
  CFileStreamBuffer m_buffer;
  IFile*            m_file;
};

}
#endif // !defined(AFX_FILE_H__A7ED6320_C362_49CB_8925_6C6C8CAE7B78__INCLUDED_)
