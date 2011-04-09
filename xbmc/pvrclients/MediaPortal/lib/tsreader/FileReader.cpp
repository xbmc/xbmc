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
#include "os-dependent.h"

FileReader::FileReader() :
  m_hFile(INVALID_HANDLE_VALUE),
  m_hInfoFile(INVALID_HANDLE_VALUE),
  m_pFileName(0),
  m_bReadOnly(false),
  m_bDelay(false),
  m_fileSize(0),
  m_infoFileSize(0),
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
  if (m_hFile != INVALID_HANDLE_VALUE) 
  {
    XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() file already open");
    return NOERROR;
  }

  // Has a filename been set yet
  if (m_pFileName == NULL) 
  {
    XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() no filename");
    return ERROR_INVALID_NAME;
  }

  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() Trying to open %s\n", m_pFileName);

  do
  {
#ifdef _WIN32
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
      if (m_hFile != INVALID_HANDLE_VALUE) break;
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
#elif defined _LINUX
    // Try to open the file
    m_hFile = open(m_pFileName,              // The filename
              O_RDONLY);                     // File access
#else
#error FIXME: Add an OpenFile() implementation for your OS
#endif
    m_bReadOnly = TRUE;
    if (m_hFile != INVALID_HANDLE_VALUE) break;

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
#ifdef _WIN32
    DWORD dwErr = GetLastError();
    XBMC->Log(LOG_DEBUG, "FileReader::OpenFile(), open file %s failed. Error code %d", m_pFileName, dwErr);
    return HRESULT_FROM_WIN32(dwErr);
#else
    XBMC->Log(LOG_DEBUG, "FileReader::OpenFile(), open file %s failed. Error %d: %s", m_pFileName, errno, strerror(errno));
    return S_FALSE;
#endif
  }

  XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() handle %i %s", m_hFile, m_pFileName );

  char infoName[512];
  strncpy(infoName, m_pFileName, 512);
  strncat(infoName, ".info", 512 - strlen(infoName) - 1);

#ifdef _WIN32
  m_hInfoFile = ::CreateFile(infoName,    // The filename
      (DWORD) GENERIC_READ,               // File access
      (DWORD) (FILE_SHARE_READ |
      FILE_SHARE_WRITE),                  // Share access
      NULL,                               // Security
      (DWORD) OPEN_EXISTING,              // Open flags
//      (DWORD) 0,
      (DWORD) FILE_ATTRIBUTE_NORMAL,      // More flags
//      FILE_FLAG_SEQUENTIAL_SCAN,        // More flags
//      FILE_ATTRIBUTE_NORMAL |
//      FILE_FLAG_RANDOM_ACCESS,          // More flags
      NULL);
#elif defined _LINUX
  m_hInfoFile = open(infoName,            // The filename
                O_RDONLY);
#else
#error FIXME: Add an OpenFile() implementation for your OS
#endif

  //XBMC->Log(LOG_DEBUG, "FileReader::OpenFile() info file handle %i", m_hInfoFile);

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

  if (m_hFile == INVALID_HANDLE_VALUE) 
  {
    //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() no open file");
    return S_OK;
  }

  //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() handle %i %ws", m_hFile, m_pFileName);
  //XBMC->Log(LOG_DEBUG, "FileReader::CloseFile() info file handle %i", m_hInfoFile);

//  BoostThread Boost;

#ifdef _WIN32
  ::CloseHandle(m_hFile);
#elif defined _LINUX
  close(m_hFile);
#else
#error FIXME: Add a CloseFile() implementation for your OS
#endif

  m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file

  if (m_hInfoFile != INVALID_HANDLE_VALUE)
  {
#ifdef _WIN32
    ::CloseHandle(m_hInfoFile);
#elif defined _LINUX
    close(m_hInfoFile);
#else
#error FIXME: Add a CloseFile() implementation for your OS
#endif
  }

  m_hInfoFile = INVALID_HANDLE_VALUE; // Invalidate the file

  m_llBufferPointer = 0;
  return NOERROR;

} // CloseFile

bool FileReader::IsFileInvalid()
{
  return (m_hFile == INVALID_HANDLE_VALUE);
}

long FileReader::GetFileSize(int64_t *pStartPosition, int64_t *pLength)
{
  //CheckPointer(pStartPosition,E_POINTER);
  //CheckPointer(pLength,E_POINTER);
  
//  BoostThread Boost;

  GetStartPosition(pStartPosition);

#ifdef _WIN32
  //Do not get file size if static file or first time 
  if (m_bReadOnly || !m_fileSize) {
    
    if (m_hInfoFile != INVALID_HANDLE_VALUE)
    {
      int64_t length = -1;
      unsigned long read = 0;
      LARGE_INTEGER li;
      li.QuadPart = 0;
      ::SetFilePointer(m_hInfoFile, li.LowPart, &li.HighPart, FILE_BEGIN);
      ::ReadFile(m_hInfoFile, (void*)&length, (DWORD)sizeof(int64_t), &read, NULL);

      if(length > -1)
      {
        m_fileSize = length;
        *pLength = length;
        return S_OK;
      }
    }

    unsigned long dwSizeLow;
    unsigned long dwSizeHigh;

    dwSizeLow = ::GetFileSize(m_hFile, &dwSizeHigh);
    if ((dwSizeLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR ))
    {
      return E_FAIL;
    }

    LARGE_INTEGER li;
    li.LowPart = dwSizeLow;
    li.HighPart = dwSizeHigh;
    m_fileSize = li.QuadPart;
  }
  *pLength = m_fileSize;
#elif defined _LINUX
#error FIXME: Finish the GetFileSize() implementation for your OS
  if (m_bReadOnly || !m_fileSize)
  {
    if (m_hInfoFile != INVALID_HANDLE_VALUE)
    {
      //TODO (see Windows implementation)
    }

    struct stat filestatus;

    if(fstat(m_hFile, &filestatus) < 0)
    {
      XBMC->Log(LOG_DEBUG, "%s: stat(%s) failed. Error %d: %s", __FUNCTION__, m_hInfoFile, errno, strerror(errno));
      return E_FAIL;
    }

    m_fileSize = filestatus.st_size;
  }
  *pLength = m_fileSize;
#else
#error FIXME: Add a GetFileSize() implementation for your OS
#endif
  return S_OK;
}

long FileReader::GetInfoFileSize(int64_t *lpllsize)
{
#ifdef _WIN32
  //Do not get file size if static file or first time 
  if (m_bReadOnly || !m_infoFileSize) {
    
    DWORD dwSizeLow;
    DWORD dwSizeHigh;

    dwSizeLow = ::GetFileSize(m_hInfoFile, &dwSizeHigh);
    if ((dwSizeLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR ))
    {
      return E_FAIL;
    }

    LARGE_INTEGER li;
    li.LowPart = dwSizeLow;
    li.HighPart = dwSizeHigh;
    m_infoFileSize = li.QuadPart;
  }
  *lpllsize = m_infoFileSize;
#elif defined _LINUX
  if (m_bReadOnly || !m_infoFileSize) {
    struct stat filestatus;

    if(fstat(m_hInfoFile, &filestatus) < 0)
    {
      XBMC->Log(LOG_DEBUG, "%s: stat(%s) failed. Error %d: %s", __FUNCTION__, m_hInfoFile, errno, strerror(errno));
      return E_FAIL;
    }

    m_infoFileSize = filestatus.st_size;
  }
  *lpllsize = m_infoFileSize;
#else
#error FIXME: Add a GetInfoFileSize() implementation for your OS
#endif
  return S_OK;
}

long FileReader::GetStartPosition(int64_t *lpllpos)
{
  //Do not get file size if static file unless first time 
  if (m_bReadOnly || !m_fileStartPos) {
    
    if (m_hInfoFile != INVALID_HANDLE_VALUE)
    {
      int64_t size = 0;
      GetInfoFileSize(&size);
      //Check if timeshift info file
      if (size > sizeof(int64_t))
      {
        //Get the file start pointer
        int64_t length = -1;
#ifdef _WIN32
        DWORD read = 0;
        LARGE_INTEGER li;
        li.QuadPart = sizeof(int64_t);
        ::SetFilePointer(m_hInfoFile, li.LowPart, &li.HighPart, FILE_BEGIN);
        ::ReadFile(m_hInfoFile, (void*)&length, (DWORD)sizeof(int64_t), &read, NULL);
#else
        //TODO: lseek (or fseek for fopen)
#error FIXME: Add a GetStartPosition() implementation for your OS
#endif

        if(length > -1)
        {
          m_fileStartPos = length;
          *lpllpos =  length;
          return S_OK;
        }
      }
    }
    m_fileStartPos = 0;
  }
  *lpllpos = m_fileStartPos;
  return S_OK;
}

unsigned long FileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
#ifdef _WIN32
  LARGE_INTEGER li;

  if (dwMoveMethod == FILE_END && m_hInfoFile != INVALID_HANDLE_VALUE)
  {
    int64_t startPos = 0;
    GetStartPosition(&startPos);

    if (startPos > 0)
    {
      int64_t start;
      int64_t fileSize = 0;
      GetFileSize(&start, &fileSize);

      int64_t filePos  = (int64_t)((int64_t)fileSize + (int64_t)llDistanceToMove + (int64_t)startPos);

      if (filePos >= fileSize)
        li.QuadPart = (int64_t)((int64_t)startPos + (int64_t)llDistanceToMove);
      else
        li.QuadPart = filePos;

      return ::SetFilePointer(m_hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
    }

    int64_t start = 0;
    int64_t length = 0;
    GetFileSize(&start, &length);

    length  = (int64_t)((int64_t)length + (int64_t)llDistanceToMove);

    li.QuadPart = length;

    dwMoveMethod = FILE_BEGIN;
  }
  else
  {
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
  }

  return ::SetFilePointer(m_hFile, li.LowPart, &li.HighPart, dwMoveMethod);
#else
  //lseek (or fseek for fopen)
#error FIXME: Add a SetFilePointer() implementation for your OS
  return 0;
#endif
}

int64_t FileReader::GetFilePointer()
{
#ifdef _WIN32
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
#else
#error FIXME: Add a GetFilePointer() implementation for your OS
  return 0;
#endif
}

long FileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  long hr;

  // If the file has already been closed, don't continue
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() no open file");
    return E_FAIL;
  }
//  BoostThread Boost;

#ifdef _WIN32
  //Get File Position
  LARGE_INTEGER li;
  li.QuadPart = 0;
  li.LowPart = ::SetFilePointer(m_hFile, 0, &li.HighPart, FILE_CURRENT);
  DWORD dwErr = ::GetLastError();
  if ((DWORD)li.LowPart == (DWORD)0xFFFFFFFF && dwErr)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() seek failed");
    return E_FAIL;
  }
  int64_t m_filecurrent = li.QuadPart;

  if (m_hInfoFile != INVALID_HANDLE_VALUE)
  {
    int64_t startPos = 0;
    GetStartPosition(&startPos);

    if (startPos > 0)
    {
      int64_t start;
      int64_t length = 0;
      GetFileSize(&start, &length);

      if (length < (int64_t)(m_filecurrent + (int64_t)lDataLength) && m_filecurrent > startPos)
      {
        hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)max(0,(length - m_filecurrent)), dwReadBytes, NULL);
        if (!hr)
          return E_FAIL;

        LARGE_INTEGER li;
        li.QuadPart = 0;
        hr = ::SetFilePointer(m_hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
        DWORD dwErr = ::GetLastError();
        if ((DWORD)hr == (DWORD)0xFFFFFFFF && dwErr)
        {
          return E_FAIL;
        }

        unsigned long dwRead = 0;

        hr = ::ReadFile(m_hFile,
          (void*)(pbData + (DWORD)max(0,(length - m_filecurrent))),
          (DWORD)max(0,((int64_t)lDataLength -(int64_t)(length - m_filecurrent))),
          &dwRead,
          NULL);

        *dwReadBytes = *dwReadBytes + dwRead;

      }
      else if (startPos < (int64_t)(m_filecurrent + (int64_t)lDataLength) && m_filecurrent < startPos)
        hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)max(0,(startPos - m_filecurrent)), dwReadBytes, NULL);

      else
        hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)lDataLength, dwReadBytes, NULL);

      if (!hr)
        return S_FALSE;

      if (*dwReadBytes < (unsigned long)lDataLength)
      {
        return E_FAIL;
      }

      return S_OK;
    }

    int64_t start = 0;
    int64_t length = 0;
    GetFileSize(&start, &length);
    if (length < (int64_t)(m_filecurrent + (int64_t)lDataLength))
      hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)max(0,(length - m_filecurrent)), dwReadBytes, NULL);
    else
      hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)lDataLength, dwReadBytes, NULL);
  }
  else
    hr = ::ReadFile(m_hFile, (void*)pbData, (DWORD)lDataLength, dwReadBytes, NULL);//Read file data into buffer

  if (!hr)
  {
    XBMC->Log(LOG_DEBUG, "FileReader::Read() read failed - error = %d",  HRESULT_FROM_WIN32(GetLastError()));
    return E_FAIL;
  }

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
  return pLength;
}
#endif //TSREADER
