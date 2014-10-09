#ifndef __ATLWFILE_H__
#define __ATLWFILE_H__

#pragma once
#include <io.h>
#include <fcntl.h>

/////////////////////////////////////////////////////////////////////////////
// Windows File API wrappers
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//


#ifndef __cplusplus
   #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef INVALID_SET_FILE_POINTER
   #define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif // INVALID_SET_FILE_POINTER

#ifndef _ASSERTE
   #define _ASSERTE(x)
#endif // _ASSERTE

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif // _ATL_DLL_IMPL


/////////////////////////////////////////////////////////////////
// Standard file wrapper

// Win32 File wrapper class
template< bool t_bManaged >
class CFileT
{
protected:
  BOOL m_bCloseOnDelete;
  CStdString m_strFileName;
public:
   HANDLE m_hFile;
   enum OpenFlags {
     modeRead =         (int) 0x00000,
     modeWrite =        (int) 0x00001,
     modeReadWrite =    (int) 0x00002,
     shareCompat =      (int) 0x00000,
     shareExclusive =   (int) 0x00010,
     shareDenyWrite =   (int) 0x00020,
     shareDenyRead =    (int) 0x00030,
     shareDenyNone =    (int) 0x00040,
     modeNoInherit =    (int) 0x00080,
     modeCreate =       (int) 0x01000,
     modeNoTruncate =   (int) 0x02000,
     typeText =         (int) 0x04000, // typeText and typeBinary are
     typeBinary =       (int) 0x08000, // used in derived classes only
     osNoBuffer =       (int) 0x10000,
     osWriteThrough =   (int) 0x20000,
     osRandomAccess =   (int) 0x40000,
     osSequentialScan = (int) 0x80000,
   };

   CFileT(HANDLE hFile = INVALID_HANDLE_VALUE) : m_hFile(hFile)
   {
   }

   CFileT(const CFileT<t_bManaged>& file) : m_hFile(INVALID_HANDLE_VALUE)
   {
      DuplicateHandle(file.m_hFile);
   }

   ~CFileT()
   { 
      if( t_bManaged ) Close(); 
   }

   operator HFILE() const 
   { 
      return (HFILE) m_hFile; 
   }

   operator HANDLE() const 
   { 
      return m_hFile; 
   }

   const CFileT<t_bManaged>& operator=(const CFileT<t_bManaged>& file)
   {
      DuplicateHandle(file.m_hFile);
      return *this;
   }

   void Abort()
   {
     if (m_hFile != INVALID_HANDLE_VALUE)
     {
       // close but ignore errors
       ::CloseHandle(m_hFile);
       m_hFile = INVALID_HANDLE_VALUE;
     }
     m_strFileName.Empty();
   }

   BOOL Open(LPCTSTR pstrFileName, UINT nOpenFlags)
   {
     ASSERT(!::IsBadStringPtr(pstrFileName,-1));

     ASSERT((nOpenFlags & typeText) == 0);   // text mode not supported

     // shouldn't open an already open file (it will leak)
     ASSERT(m_hFile == INVALID_HANDLE_VALUE);

     // CFile objects are always binary and CreateFile does not need flag
     nOpenFlags &= ~(UINT)typeBinary;

     m_bCloseOnDelete = FALSE;

     m_hFile = INVALID_HANDLE_VALUE;
     m_strFileName.Empty();

     m_strFileName = pstrFileName;
     ASSERT(shareCompat == 0);

     // map read/write mode
     ASSERT((modeRead|modeWrite|modeReadWrite) == 3);
     DWORD dwAccess = 0;
     switch (nOpenFlags & 3)
     {
     case modeRead:
       dwAccess = GENERIC_READ;
       break;
     case modeWrite:
       dwAccess = GENERIC_WRITE;
       break;
     case modeReadWrite:
       dwAccess = GENERIC_READ | GENERIC_WRITE;
       break;
     default:
       ASSERT(FALSE);  // invalid share mode
     }

     // map share mode
     DWORD dwShareMode = 0;
     switch (nOpenFlags & 0x70)    // map compatibility mode to exclusive
     {
     default:
       ASSERT(FALSE);  // invalid share mode?
     case shareCompat:
     case shareExclusive:
       dwShareMode = 0;
       break;
     case shareDenyWrite:
       dwShareMode = FILE_SHARE_READ;
       break;
     case shareDenyRead:
       dwShareMode = FILE_SHARE_WRITE;
       break;
     case shareDenyNone:
       dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
       break;
     }

     // Note: typeText and typeBinary are used in derived classes only.

     // map modeNoInherit flag
     SECURITY_ATTRIBUTES sa;
     sa.nLength = sizeof(sa);
     sa.lpSecurityDescriptor = NULL;
     sa.bInheritHandle = (nOpenFlags & modeNoInherit) == 0;

     // map creation flags
     DWORD dwCreateFlag;
     if (nOpenFlags & modeCreate)
     {
       if (nOpenFlags & modeNoTruncate)
         dwCreateFlag = OPEN_ALWAYS;
       else
         dwCreateFlag = CREATE_ALWAYS;
     }
     else
       dwCreateFlag = OPEN_EXISTING;

     // special system-level access flags

     // Random access and sequential scan should be mutually exclusive
     ASSERT((nOpenFlags&(osRandomAccess|osSequentialScan)) != (osRandomAccess|
       osSequentialScan) );

     DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
     if (nOpenFlags & osNoBuffer)
       dwFlags |= FILE_FLAG_NO_BUFFERING;
     if (nOpenFlags & osWriteThrough)
       dwFlags |= FILE_FLAG_WRITE_THROUGH;
     if (nOpenFlags & osRandomAccess)
       dwFlags |= FILE_FLAG_RANDOM_ACCESS;
     if (nOpenFlags & osSequentialScan)
       dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

     // attempt file creation
     HANDLE hFile = ::CreateFile(pstrFileName, dwAccess, dwShareMode, &sa,
       dwCreateFlag, dwFlags, NULL);
     if (hFile == INVALID_HANDLE_VALUE)
     {
       return FALSE;
     }
     m_hFile = hFile;
     m_bCloseOnDelete = TRUE;

     return TRUE;
   }
   
   BOOL Create(LPCTSTR pstrFileName,
               DWORD dwAccess = GENERIC_WRITE, 
               DWORD dwShareMode = 0 /*DENY ALL*/, 
               DWORD dwFlags = CREATE_ALWAYS,
               DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL)
   {
      return Open(pstrFileName, dwAccess, dwShareMode, dwFlags, dwAttributes);
   }
   
   void Close()
   {
      if( m_hFile == INVALID_HANDLE_VALUE ) return;
      ::CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE;
   }
   
   BOOL IsOpen() const
   {
      return m_hFile != INVALID_HANDLE_VALUE;
   }
   
   void Attach(HANDLE hHandle)
   {
      Close();
      m_hFile = hHandle;
   }   
   
   HANDLE Detach()
   {
      HANDLE h = m_hFile;
      m_hFile = INVALID_HANDLE_VALUE;
      return h;
   }
   
   DWORD Read(LPVOID lpBuf, DWORD nCount)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      _ASSERTE(lpBuf!=NULL);
      _ASSERTE(!::IsBadWritePtr(lpBuf, nCount));
      if( nCount == 0 ) return TRUE;   // avoid Win32 "null-read"
      DWORD dwBytesRead = 0;
      if( !::ReadFile(m_hFile, lpBuf, nCount, &dwBytesRead, NULL) ) return 0;
      return dwBytesRead;
   }
   
   BOOL Read(LPVOID lpBuf, DWORD nCount, LPDWORD pdwRead)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      _ASSERTE(lpBuf);
      _ASSERTE(!::IsBadWritePtr(lpBuf, nCount));
      _ASSERTE(pdwRead);
      *pdwRead = 0;
      if( nCount == 0 ) return TRUE;   // avoid Win32 "null-read"
      if( !::ReadFile(m_hFile, lpBuf, nCount, pdwRead, NULL) ) return FALSE;
      return TRUE;
   }
   
   DWORD Write(LPCVOID lpBuf, DWORD nCount)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      _ASSERTE(lpBuf!=NULL);
      _ASSERTE(!::IsBadReadPtr(lpBuf, nCount));   
      if( nCount == 0 ) return TRUE; // avoid Win32 "null-write" option
      DWORD dwBytesWritten = 0;
      if( !::WriteFile(m_hFile, lpBuf, nCount, &dwBytesWritten, NULL) ) return 0;
      return dwBytesWritten;
   }
   
   BOOL Write(LPCVOID lpBuf, DWORD nCount, LPDWORD pdwWritten)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      _ASSERTE(lpBuf);
      _ASSERTE(!::IsBadReadPtr(lpBuf, nCount));
      _ASSERTE(pdwWritten);    
      *pdwWritten = 0;
      if( nCount == 0 ) return TRUE; // avoid Win32 "null-write" option
      if( !::WriteFile(m_hFile, lpBuf, nCount, pdwWritten, NULL) ) return FALSE;
      return TRUE;
   }
   
   DWORD Seek(LONG lOff, UINT nFrom)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::SetFilePointer(m_hFile, lOff, NULL, (DWORD) nFrom);
   }

   DWORD GetPosition() const
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT);
   }

   BOOL Lock(DWORD dwOffset, DWORD dwSize)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::LockFile(m_hFile, dwOffset, 0, dwSize, 0);
   }

   BOOL Unlock(DWORD dwOffset, DWORD dwSize)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::UnlockFile(m_hFile, dwOffset, 0, dwSize, 0);
   }

   BOOL SetEOF()
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::SetEndOfFile(m_hFile);
   }

   BOOL Flush()
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::FlushFileBuffers(m_hFile);
   }
   
   DWORD GetSize() const
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::GetFileSize(m_hFile, NULL);
   }

   DWORD GetType() const
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::GetFileType(m_hFile);
   }

   BOOL GetFileTime(FILETIME* ftCreate, FILETIME* ftAccess, FILETIME* ftModified)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::GetFileTime(m_hFile, ftCreate, ftAccess, ftModified);
   }

   BOOL SetFileTime(FILETIME* ftCreate, FILETIME* ftAccess, FILETIME* ftModified)
   {
      _ASSERTE(m_hFile!=INVALID_HANDLE_VALUE);
      return ::SetFileTime(m_hFile, ftCreate, ftAccess, ftModified);
   }

   BOOL DuplicateHandle(HANDLE hOther)
   {
      ATLASSERT(m_hFile==INVALID_HANDLE_VALUE);
      ATLASSERT(hOther!=INVALID_HANDLE_VALUE);
      HANDLE process = ::GetCurrentProcess();
      BOOL res = ::DuplicateHandle(process, hOther, process, &m_hFile, NULL, FALSE, DUPLICATE_SAME_ACCESS);
      _ASSERTE(res);
      return res;
   }

   // Static members

   static BOOL FileExists(LPCTSTR pstrFileName)
   {
      _ASSERTE(!::IsBadStringPtr(pstrFileName, MAX_PATH));
      DWORD dwErrMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
      DWORD dwAttribs = ::GetFileAttributes(pstrFileName);
      ::SetErrorMode(dwErrMode);
      return (dwAttribs != (DWORD) -1) && (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
   }

   static BOOL Delete(LPCTSTR pstrFileName)
   {
      _ASSERTE(!::IsBadStringPtr(pstrFileName, MAX_PATH));
      return ::DeleteFile(pstrFileName);
   }
   
   static BOOL Rename(LPCTSTR pstrSourceFileName, LPCTSTR pstrTargetFileName)
   {
      _ASSERTE(!::IsBadStringPtr(pstrSourceFileName, MAX_PATH));
      _ASSERTE(!::IsBadStringPtr(pstrTargetFileName, MAX_PATH));
      return ::MoveFile(pstrSourceFileName, pstrTargetFileName);
   }
};

typedef CFileT<true> CFile;
typedef CFileT<false> CFileHandle;

class CStdioFile: public CFile
{
public:
  // Constructors
  CStdioFile()
    : m_pStream(NULL)
  {

  }
  CStdioFile(FILE* pOpenStream)
  {
    ASSERT(pOpenStream != NULL);

    if (pOpenStream == NULL)
    {
      return;
    }

    m_pStream = pOpenStream;
    m_hFile = (HANDLE) _get_osfhandle(_fileno(pOpenStream));
    ASSERT(!m_bCloseOnDelete);
  }
  CStdioFile(LPCTSTR lpszFileName, UINT nOpenFlags)
  {
    ASSERT(lpszFileName != NULL);

    if (lpszFileName == NULL)
    {
      return;
    }

    if (!Open(lpszFileName, nOpenFlags))
      return;
  }

  // Attributes
  FILE* m_pStream;    // stdio FILE
  // m_hFile from base class is _fileno(m_pStream)

  // Operations
  // reading and writing strings
  virtual void WriteString(LPCTSTR lpsz)
  {
    ASSERT(lpsz != NULL);
    ASSERT(m_pStream != NULL);

    if (lpsz == NULL)
    {
      return;
    }
    CStdStringA foo(lpsz);
    if (fputs(foo.c_str(), m_pStream) == _TEOF)
      return;
  }
  virtual LPTSTR ReadString(_Out_z_cap_(nMax) LPTSTR lpsz, _In_ UINT nMax)
  {
    ASSERT(lpsz != NULL);
    ASSERT(m_pStream != NULL);

    if (lpsz == NULL)
    {
      return NULL;
    }

    CStdStringA foo;
    char* lpszResult = fgets(foo.GetBuffer(nMax), nMax, m_pStream);
    if (lpszResult == NULL && !feof(m_pStream))
    {
      ::clearerr_s(m_pStream);
      return NULL;
    }
    memcpy((void*) lpsz, (void*) foo.GetBuffer(), foo.length());
    return lpsz;
  }
  virtual BOOL ReadString(CStdString& rString)
  {
    rString = _T("");    // empty string without deallocating
    const int nMaxSize = 128;
    CStdStringA aString = CStdStringA(rString);
    char* lpsz = aString.GetBuffer(nMaxSize);
    char* lpszResult;
    int nLen = 0;
    for (;;)
    {
      lpszResult = fgets(lpsz, nMaxSize+1, m_pStream);
      aString.ReleaseBuffer();

      // handle error/eof case
      if (lpszResult == NULL && !feof(m_pStream))
      {
        ::clearerr_s(m_pStream);
        return FALSE;
      }

      // if string is read completely or EOF
      if (lpszResult == NULL ||
        (nLen = (int)lstrlenA(lpsz)) < nMaxSize ||
        lpsz[nLen-1] == '\n')
        break;

      nLen = aString.GetLength();
      lpsz = aString.GetBuffer(nMaxSize + nLen) + nLen;
    }

    // remove '\n' from end of string if present
    lpsz = aString.GetBuffer(0);
    nLen = aString.GetLength();
    if (nLen != 0 && lpsz[nLen-1] == '\n')
      aString.GetBufferSetLength(nLen-1);

    rString = aString;
    return nLen != 0;
  }

  // Implementation
public:
  virtual ~CStdioFile()
  {
    if (m_pStream != NULL && m_bCloseOnDelete)
      Close();
  }
  virtual ULONGLONG GetPosition() const
  {
    ASSERT(m_pStream != NULL);

    long pos = ftell(m_pStream);
    if (pos == -1)
      return 0;
    return pos;
  }
  virtual ULONGLONG GetLength() const
  {
    LONG nCurrent;
    LONG nLength;
    LONG nResult;

    nCurrent = ftell(m_pStream);
    if (nCurrent == -1)
      return 0;

    nResult = fseek(m_pStream, 0, SEEK_END);
    if (nResult != 0)
      return 0;

    nLength = ftell(m_pStream);
    if (nLength == -1)
      return 0;
    nResult = fseek(m_pStream, nCurrent, SEEK_SET);
    if (nResult != 0)
      return 0;

    return nLength;
  }
  virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags)
  {
    ASSERT(lpszFileName != NULL);

    if (lpszFileName == NULL)
    {
      return FALSE;
    }

    m_pStream = NULL;
    //if (!CFile::Open(lpszFileName, (nOpenFlags & ~typeText)))
    //  return FALSE;

    //ASSERT(m_hFile != INVALID_HANDLE_VALUE);
    //ASSERT(m_bCloseOnDelete);
    m_bCloseOnDelete = TRUE;

    wchar_t szMode[4]; // C-runtime open string
    int nMode = 0;

    // determine read/write mode depending on CFile mode
    if (nOpenFlags & modeCreate)
    {
      if (nOpenFlags & modeNoTruncate)
        szMode[nMode++] = 'a';
      else
        szMode[nMode++] = 'w';
    }
    else if (nOpenFlags & modeWrite)
      szMode[nMode++] = 'a';
    else
      szMode[nMode++] = 'r';

    // add '+' if necessary (when read/write modes mismatched)
    if (szMode[0] == 'r' && (nOpenFlags & modeReadWrite) ||
      szMode[0] != 'r' && !(nOpenFlags & modeWrite))
    {
      // current szMode mismatched, need to add '+' to fix
      szMode[nMode++] = '+';
    }

    // will be inverted if not necessary
    int nFlags = _O_RDONLY | _O_TEXT;
    if (nOpenFlags & (modeWrite|modeReadWrite))
      nFlags ^= _O_RDONLY;

    if (nOpenFlags & typeBinary)
      szMode[nMode++] = 'b', nFlags ^= _O_TEXT;
    else
      szMode[nMode++] = 't';
    szMode[nMode++] = '\0';

    // open a C-runtime low-level file handle
    m_pStream = _wfopen(lpszFileName, szMode);

    if (m_pStream == NULL)
    {
      CFile::Abort(); // close m_hFile
      return FALSE;
    }

    return TRUE;
  }
  virtual UINT Read(void* lpBuf, UINT nCount)
  {
    ASSERT(m_pStream != NULL);

    if (nCount == 0)
      return 0;   // avoid Win32 "null-read"

    if (lpBuf == NULL)
    {
      return 0;
    }

    UINT nRead = (UINT)fread(lpBuf, sizeof(BYTE), nCount, m_pStream);
    int error = ferror(m_pStream);

    if (nRead == 0 && !feof(m_pStream))
      return 0;
    if (ferror(m_pStream))
    {
      ::clearerr_s(m_pStream);
      return 0;
    }
    return nRead;
  }
  virtual void Write(const void* lpBuf, UINT nCount)
  {
    ASSERT(m_pStream != NULL);

    if (lpBuf == NULL)
    {
      return;
    }

    if (fwrite(lpBuf, sizeof(BYTE), nCount, m_pStream) != nCount)
      return;
  }
  virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom)
  {
    ASSERT(nFrom == FILE_BEGIN || nFrom == FILE_END || nFrom == FILE_CURRENT);
    ASSERT(m_pStream != NULL);

    LONG lOff32;

    if ((lOff < LONG_MIN) || (lOff > LONG_MAX))
    {
      return 0;
    }

    lOff32 = (LONG)lOff;
    if (fseek(m_pStream, lOff32, nFrom) != 0)
      return 0;

    long pos = ftell(m_pStream);
    return pos;
  }
  virtual void Abort()
  {
    if (m_pStream != NULL && m_bCloseOnDelete)
      fclose(m_pStream);  // close but ignore errors
    m_hFile = INVALID_HANDLE_VALUE;
    m_pStream = NULL;
    m_bCloseOnDelete = FALSE;
  }
  virtual void Flush()
  {
    if (m_pStream != NULL && fflush(m_pStream) != 0)
      return;
  }
  virtual void Close()
  {
    ASSERT(m_pStream != NULL);

    int nErr = 0;

    if (m_pStream != NULL)
      nErr = fclose(m_pStream);

    m_hFile = INVALID_HANDLE_VALUE;
    m_bCloseOnDelete = FALSE;
    m_pStream = NULL;

    if (nErr != 0)
      return;
  }
};


#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif // _ATL_DLL_IMPL


#endif // __ATLWFILE_H___
