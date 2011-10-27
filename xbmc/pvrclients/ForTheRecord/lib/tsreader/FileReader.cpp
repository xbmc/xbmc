/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  This file originates from TSFileSource, a GPL directshow push
 *  source filter that provides an MPEG transport stream output.
 *  Copyright (C) 2005      nate
 *  Copyright (C) 2006      bear
 *
 *  nate can be reached on the forums at
 *    http://forums.dvbowners.com/
 */

#if defined TSREADER

#include "FileReader.h"
#include "client.h" //for XBMC->Log
#include "libPlatform/os-dependent.h"
#if !defined(TARGET_WINDOWS)
#include "PlatformInclude.h"
#include "limits.h"
#define ERROR_FILENAME_EXCED_RANGE (206)
#define ERROR_INVALID_NAME (123)
using namespace XFILE;
#endif

using namespace ADDON;

FileReader::FileReader() :
#if defined(TARGET_WINDOWS)
  m_hFile(INVALID_HANDLE_VALUE),
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
m_hFile(),
#else
#error Implement initialisation of file for your OS
#endif
  m_pFileName(0),
  m_bReadOnly(false),
  m_bDelay(false),
  m_fileSize(0),
  m_fileStartPos(0),
  m_llBufferPointer(0),
  m_bDebugOutput(false)
{
}

FileReader::~FileReader()
{
  CloseFile();
  if (m_pFileName)
    delete m_pFileName;
}


long FileReader::GetFileName(char* *lpszFileName)
{
  *lpszFileName = m_pFileName;
  return S_OK;
}

long FileReader::SetFileName(const char *pszFileName)
{
  // Is this a valid filename supplied
  //CheckPointer(pszFileName,E_POINTER);

  if(strlen(pszFileName) > MAX_PATH)
    return ERROR_FILENAME_EXCED_RANGE;

  // Take a copy of the filename

  if (m_pFileName)
  {
    delete[] m_pFileName;
    m_pFileName = NULL;
  }
  m_pFileName = new char[1 + strlen(pszFileName)];
  if (m_pFileName == NULL)
    return E_OUTOFMEMORY;

  strncpy(m_pFileName, pszFileName, strlen(pszFileName) + 1);

  return S_OK;
}

//
// OpenFile
//
// Opens the file ready for streaming
//
long FileReader::OpenFile()
{
  //char *pFileName = NULL;
  int Tmo=25 ; //5 in MediaPortal
  // Is the file already opened
  if (!IsFileInvalid())

  {
    XBMC->Log(LOG_NOTICE, "FileReader::OpenFile() file already open");
    return NOERROR;
  }

  // Has a filename been set yet
  if (m_pFileName == NULL) 
  {
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile() no filename");
    return ERROR_INVALID_NAME;
  }

  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() Trying to open %s\n", m_pFileName);

  do
  {
#if defined(TARGET_WINDOWS)
    // do not try to open a tsbuffer file without SHARE_WRITE so skip this try if we have a buffer file
    if (strstr(m_pFileName, ".ts.tsbuffer") == NULL) 
    {
      // Try to open the file
      m_hFile = ::CreateFile(m_pFileName,      // The filename
             (DWORD) GENERIC_READ,             // File access
             (DWORD) FILE_SHARE_READ,          // Share access
             NULL,                             // Security
             (DWORD) OPEN_EXISTING,            // Open flags
             (DWORD) 0,                        // More flags
             NULL);                            // Template

      m_bReadOnly = FALSE;
      if (!IsFileInvalid()) break;
    }

    //Test incase file is being recorded to
    m_hFile = ::CreateFile(m_pFileName,         // The filename
              (DWORD) GENERIC_READ,             // File access
              (DWORD) (FILE_SHARE_READ |
              FILE_SHARE_WRITE),                // Share access
              NULL,                             // Security
              (DWORD) OPEN_EXISTING,            // Open flags
//              (DWORD) 0,
              (DWORD) FILE_ATTRIBUTE_NORMAL,    // More flags
//              FILE_ATTRIBUTE_NORMAL |
//              FILE_FLAG_RANDOM_ACCESS,        // More flags
//              FILE_FLAG_SEQUENTIAL_SCAN,      // More flags
              NULL);                            // Template
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
    // Try to open the file
    XBMC->Log(LOG_INFO, "FileReader::OpenFile() %s %s.", m_pFileName, CFile::Exists(m_pFileName) ? "exists" : "not found");
    m_hFile.Open(m_pFileName, READ_CHUNKED);        // Open in readonly mode with this filename
#else
#error FIXME: Add an OpenFile() implementation for your OS
#endif
    m_bReadOnly = TRUE;
    if (!IsFileInvalid()) break;

    Sleep(20) ;
  }
  while(--Tmo) ;
  if (Tmo)
  {
    if (Tmo<4) // 1 failed + 1 succeded is quasi-normal, more is a bit suspicious ( disk drive too slow or problem ? )
      XBMC->Log(LOG_DEBUG, "FileReader::OpenFile(), %d tries to succeed opening %ws.", 6-Tmo, m_pFileName);
  }
  else
  {
#ifdef TARGET_WINDOWS
    DWORD dwErr = GetLastError();
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile(), open file %s failed. Error code %d", m_pFileName, dwErr);
    return HRESULT_FROM_WIN32(dwErr);
#else
    XBMC->Log(LOG_ERROR, "FileReader::OpenFile(), open file %s failed. Error %d: %s", m_pFileName, errno, strerror(errno));
    return S_FALSE;
#endif
  }

#if defined(TARGET_WINDOWS)
  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() %s handle %i %s", m_bReadOnly ? "read-only" : "read/write", m_hFile, m_pFileName );
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() %s handle %p %s", m_bReadOnly ? "read-only" : "read/write", m_hFile.GetImplemenation(), m_pFileName );
#else
#error FIXME: Add an debug log implementation for your OS
#endif

  SetFilePointer(0, FILE_BEGIN);
  m_llBufferPointer = 0;

  return S_OK;

} // Open

//
// CloseFile
//
// Closes any dump file we have opened
//
long FileReader::CloseFile()
{
  // Must lock this section to prevent problems related to
  // closing the file while still receiving data in Receive()

  if (IsFileInvalid())
  {
    //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() no open file");
    return S_OK;
  }

  //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() handle %i %ws", m_hFile, m_pFileName);

//  BoostThread Boost;

#if defined(TARGET_WINDOWS)
  ::CloseHandle(m_hFile);
  m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  m_hFile.Close();
#else
#error FIXME: Add a CloseFile() implementation for your OS
#endif

  m_llBufferPointer = 0;
  return NOERROR;
} // CloseFile

bool FileReader::IsFileInvalid()
{
#if defined(TARGET_WINDOWS)
  return (m_hFile == INVALID_HANDLE_VALUE);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  return (m_hFile.GetImplemenation() == NULL);
#else
#error FIXME: Add an IsFileInvalid implementation for your OS
#endif
}

long FileReader::GetFileSize(int64_t *pStartPosition, int64_t *pLength)
{
  //CheckPointer(pStartPosition,E_POINTER);
  //CheckPointer(pLength,E_POINTER);
  
//  BoostThread Boost;

  GetStartPosition(pStartPosition);

#if defined(TARGET_WINDOWS)
  //Do not get file size if static file or first time 
  if (m_bReadOnly || !m_fileSize) {
    unsigned long dwSizeLow;
    unsigned long dwSizeHigh;

    dwSizeLow = ::GetFileSize(m_hFile, &dwSizeHigh);
    DWORD dwErr = GetLastError();
    if ((dwSizeLow == 0xFFFFFFFF) && (dwErr != NO_ERROR ))
    {
      XBMC->Log(LOG_ERROR, "%s: GetFileSize(File) failed. Error %d.", __FUNCTION__, dwErr);
      return E_FAIL;
    }

    LARGE_INTEGER li;
    li.LowPart = dwSizeLow;
    li.HighPart = dwSizeHigh;
    m_fileSize = li.QuadPart;
  }
  *pLength = m_fileSize;
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  if (m_bReadOnly || !m_fileSize)
  {
    struct stat64 filestatus;

    if(m_hFile.Stat(&filestatus) < 0)
    {
      XBMC->Log(LOG_ERROR, "%s: fstat64(File) failed. Error %d: %s", __FUNCTION__, errno, strerror(errno));
      return E_FAIL;
    }

    m_fileSize = filestatus.st_size;
  }
  *pLength = m_fileSize;
#else
#error FIXME: Add a GetFileSize() implementation for your OS
#endif
  //XBMC->Log(LOG_DEBUG, "%s: returns %d.", __FUNCTION__, m_fileSize);
  return S_OK;
}

long FileReader::GetStartPosition(int64_t *lpllpos)
{
  //Do not get file size if static file unless first time 
  if (m_bReadOnly || !m_fileStartPos) {
    m_fileStartPos = 0;
  }
  *lpllpos = m_fileStartPos;
  //XBMC->Log(LOG_DEBUG, "%s: returns %d.", __FUNCTION__, m_fileStartPos);
  return S_OK;
}

unsigned long FileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod);
#if defined(TARGET_WINDOWS)
  LARGE_INTEGER li;

	int64_t startPos = 0;
	GetStartPosition(&startPos);

  if (startPos > 0)
  {
    int64_t start;
    int64_t fileSize = 0;
    GetFileSize(&start, &fileSize);

    int64_t filePos  = (int64_t)((int64_t)startPos + (int64_t)llDistanceToMove);

    if (filePos >= fileSize)
    li.QuadPart = (int64_t)((int64_t)filePos - (int64_t)fileSize);
    else
    li.QuadPart = filePos;

    return ::SetFilePointer(m_hFile, li.LowPart, &li.HighPart, dwMoveMethod);
  }
  li.QuadPart = llDistanceToMove;

  return ::SetFilePointer(m_hFile, li.LowPart, &li.HighPart, dwMoveMethod);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  // Stupid but simple movement transform
  if (dwMoveMethod == FILE_BEGIN) dwMoveMethod = SEEK_SET;
  else if (dwMoveMethod == FILE_CURRENT) dwMoveMethod = SEEK_CUR;
  else if (dwMoveMethod == FILE_END) dwMoveMethod = SEEK_END;
  else
  {
      XBMC->Log(LOG_ERROR, "%s: SetFilePointer invalid MoveMethod(%d)", __FUNCTION__, dwMoveMethod);
	  dwMoveMethod = SEEK_SET;
  }
  off64_t myOffset;
  int64_t startPos = 0;
  GetStartPosition(&startPos);

  if (startPos > 0)
  {
    int64_t start;
    int64_t fileSize = 0;
    GetFileSize(&start, &fileSize);

    int64_t filePos  = (int64_t)((int64_t)startPos + (int64_t)llDistanceToMove);

    if (filePos >= fileSize)
      myOffset = (int64_t)((int64_t)filePos - (int64_t)fileSize);
    else
      myOffset = filePos;

    int64_t rc = m_hFile.Seek(myOffset, dwMoveMethod);
    //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d returns %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod, rc);
    return rc;
  }
  myOffset = llDistanceToMove;
  int64_t rc = m_hFile.Seek(myOffset, dwMoveMethod);
  //XBMC->Log(LOG_DEBUG, "%s: distance %d method %d returns %d.", __FUNCTION__, llDistanceToMove, dwMoveMethod, rc);
  return rc;
#else
  //lseek (or fseek for fopen)
#error FIXME: Add a SetFilePointer() implementation for your OS
  return 0;
#endif
}

int64_t FileReader::GetFilePointer()
{
#if defined(TARGET_WINDOWS)
  LARGE_INTEGER li;
  li.QuadPart = 0;
  li.LowPart = ::SetFilePointer(m_hFile, 0, &li.HighPart, FILE_CURRENT);

  int64_t start;
  int64_t length = 0;
  GetFileSize(&start, &length);

  int64_t startPos = 0;
  GetStartPosition(&startPos);

  if (startPos > 0)
  {
    if(startPos > (int64_t)li.QuadPart)
      li.QuadPart = (int64_t)(length - startPos + (int64_t)li.QuadPart);
    else
      li.QuadPart = (int64_t)((int64_t)li.QuadPart - startPos);
  }

  return li.QuadPart;
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  off64_t myOffset;
  myOffset = m_hFile.Seek(0, SEEK_CUR);

  int64_t startPos = 0;
  int64_t length = 0;
  GetFileSize(&startPos, &length);

  if (startPos > 0)
  {
    if(startPos > (int64_t)myOffset)
      myOffset = (int64_t)(length - startPos + (int64_t)myOffset);
    else
      myOffset = (int64_t)((int64_t)myOffset - startPos);
  }

  //XBMC->Log(LOG_DEBUG, "%s: returns %d GetPosition(%d).", __FUNCTION__, myOffset, m_hFile.GetPosition());
  return myOffset;
#else
#error FIXME: Add a GetFilePointer() implementation for your OS
  return 0;
#endif
}

long FileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  // If the file has already been closed, don't continue
  if (IsFileInvalid())
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() no open file");
    return E_FAIL;
  }
//  BoostThread Boost;

#ifdef TARGET_WINDOWS
  long hr;

  //Get File Position
  LARGE_INTEGER li;
  li.QuadPart = 0;
  li.LowPart = ::SetFilePointer(m_hFile, 0, &li.HighPart, FILE_CURRENT);
  DWORD dwErr = ::GetLastError();
  if ((DWORD)li.LowPart == (DWORD)0xFFFFFFFF && dwErr)
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() seek failed, error %d.", dwErr);
    return E_FAIL;
  }
  int64_t m_filecurrent = li.QuadPart;

  hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)lDataLength, dwReadBytes, NULL);//Read file data into buffer

  if (!hr)
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() read failed - error = %d",  HRESULT_FROM_WIN32(GetLastError()));
    return E_FAIL;
  }

  if (*dwReadBytes < (unsigned long)lDataLength)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() read to less bytes");
    return S_FALSE;
  }
  return S_OK;
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  *dwReadBytes = m_hFile.Read((void*)pbData, (DWORD)lDataLength);//Read file data into buffer
  //XBMC->Log(LOG_DEBUG, "%s: requested read length %d actually read %d.", __FUNCTION__, lDataLength, *dwReadBytes);

  if (*dwReadBytes < (unsigned long)lDataLength)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() read to less bytes");
    return S_FALSE;
  }
  return S_OK;
#else
#error FIXME: Add a Read() implementation for your OS
  return S_FALSE;
#endif
}

long FileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes, int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //If end method then we want llDistanceToMove to be the end of the buffer that we read.
  if (dwMoveMethod == FILE_END)
    llDistanceToMove = 0 - llDistanceToMove - lDataLength;

  SetFilePointer(llDistanceToMove, dwMoveMethod);

  return Read(pbData, lDataLength, dwReadBytes);
}

long FileReader::get_ReadOnly(bool *ReadOnly)
{
  *ReadOnly = m_bReadOnly;
  return S_OK;
}

long FileReader::get_DelayMode(bool *DelayMode)
{
  *DelayMode = m_bDelay;
  return S_OK;
}

long FileReader::set_DelayMode(bool DelayMode)
{
  m_bDelay = DelayMode;
  return S_OK;
}

long FileReader::get_ReaderMode(unsigned short *ReaderMode)
{
  *ReaderMode = FALSE;
  return S_OK;
}

void FileReader::SetDebugOutput(bool bDebugOutput)
{
  m_bDebugOutput = bDebugOutput;
}

unsigned long FileReader::setFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //Get the file information
  int64_t fileStart, fileEnd, fileLength;
  GetFileSize(&fileStart, &fileLength);
  fileEnd = fileLength;
  if (dwMoveMethod == FILE_BEGIN)
    return SetFilePointer((int64_t)min(fileEnd, llDistanceToMove), FILE_BEGIN);
  else
    return SetFilePointer((int64_t)max((int64_t)-fileLength, llDistanceToMove), FILE_END);
}

int64_t FileReader::getFilePointer()
{
  return GetFilePointer();
}

int64_t FileReader::getBufferPointer()
{
  return   m_llBufferPointer;
}

void FileReader::setBufferPointer()
{
  m_llBufferPointer = GetFilePointer();
}

int64_t FileReader::GetFileSize()
{
  int64_t pStartPosition =0;
  int64_t pLength=0;
  GetFileSize(&pStartPosition, &pLength);
  //XBMC->Log(LOG_DEBUG, "%s: returns %d, GetLength(%d).", __FUNCTION__, pLength, m_hFile.GetLength());
  return pLength;
}
#endif //TSREADER
