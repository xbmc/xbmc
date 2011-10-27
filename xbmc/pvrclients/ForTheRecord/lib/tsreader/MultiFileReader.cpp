/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#ifdef TSREADER

#include "MultiFileReader.h"
#include "client.h" //for XBMC->Log
#include <string>
#include "utils.h"
#include <wchar.h>
#if !defined(TARGET_WINDOWS)
#include <sys/time.h>
#include "PlatformInclude.h"
#include "File.h"
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
using namespace XFILE;
#endif

using namespace ADDON;

//Maximum time in msec to wait for the buffer file to become available - Needed for DVB radio (this sometimes takes some time)
#define MAX_BUFFER_TIMEOUT 1500

MultiFileReader::MultiFileReader():
  m_TSBufferFile(),
  m_TSFile()
{
  m_startPosition = 0;
  m_endPosition = 0;
  m_currentPosition = 0;
  m_filesAdded = 0;
  m_filesRemoved = 0;
  m_TSFileId = 0;
  m_bReadOnly = 1;
  m_bDelay = 0;
  m_llBufferPointer = 0;
  m_cachedFileSize = 0;
  m_bDebugOutput = false;
}

MultiFileReader::~MultiFileReader()
{
  //CloseFile called by ~FileReader
/*  USES_CONVERSION;

  std::vector<MultiFileReaderFile *>::iterator it = m_tsFiles.begin();
  for ( ; it < m_tsFiles.end() ; it++ )
  {
    if((*it)->filename)
    {
      DeleteFile(W2T((*it)->filename));
      delete[] (*it)->filename;
    }

    delete *it;
  };
*/
}


long MultiFileReader::GetFileName(char* *lpszFileName)
{
//  CheckPointer(lpszFileName,E_POINTER);
  return m_TSBufferFile.GetFileName(lpszFileName);
}

long MultiFileReader::SetFileName(const char* pszFileName)
{
//  CheckPointer(pszFileName,E_POINTER);
  return m_TSBufferFile.SetFileName(pszFileName);
}

//
// OpenFile
//
long MultiFileReader::OpenFile()
{
  long hr = m_TSBufferFile.OpenFile();

  //For radio the buffer sometimes needs some time to become available, so wait try it more than once
#if defined(TARGET_WINDOWS)
  unsigned long tc=GetTickCount();
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  struct timeval tstart;
  gettimeofday(&tstart, NULL);
#else
#error FIXME: Add some form of timeout handling for your OS
#endif
  while (RefreshTSBufferFile()==S_FALSE)
  {
#if defined(TARGET_WINDOWS)
    if (GetTickCount()-tc>MAX_BUFFER_TIMEOUT)
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
    struct timeval tnow, tdelta;
    gettimeofday(&tnow, NULL);
    timersub(&tnow, &tstart, &tdelta);
    if ((tdelta.tv_sec * 1000) + (tdelta.tv_usec / 1000) > MAX_BUFFER_TIMEOUT)
#else
#error FIXME: Add some form of timeout handling for your OS
#endif
    {
      XBMC->Log(LOG_ERROR, "MultiFileReader: timedout while waiting for buffer file to become available");
      XBMC->QueueNotification(QUEUE_ERROR, "Time out while waiting for buffer file");
      return S_FALSE;
    }
  }

  m_currentPosition = 0;
  m_llBufferPointer = 0;

  return hr;
}

//
// CloseFile
//
long MultiFileReader::CloseFile()
{
  long hr;
  hr = m_TSBufferFile.CloseFile();
  hr = m_TSFile.CloseFile();
  m_TSFileId = 0;
  m_llBufferPointer = 0;
  return hr;
}

bool MultiFileReader::IsFileInvalid()
{
  return m_TSBufferFile.IsFileInvalid();
}

long MultiFileReader::GetFileSize(int64_t *pStartPosition, int64_t *pLength)
{
//  RefreshTSBufferFile();
//  CheckPointer(pStartPosition,E_POINTER);
//  CheckPointer(pLength,E_POINTER);
  *pStartPosition = m_startPosition;
  *pLength = (int64_t)(m_endPosition - m_startPosition);
  return S_OK;
}

unsigned long MultiFileReader::SetFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
//  RefreshTSBufferFile();

  if (dwMoveMethod == FILE_END)
  {
    m_currentPosition = m_endPosition + llDistanceToMove;
  }
  else if (dwMoveMethod == FILE_CURRENT)
  {
    m_currentPosition += llDistanceToMove;
  }
  else // if (dwMoveMethod == FILE_BEGIN)
  {
    m_currentPosition = m_startPosition + llDistanceToMove;
  }

  if (m_currentPosition < m_startPosition)
    m_currentPosition = m_startPosition;

  if (m_currentPosition > m_endPosition) {
    XBMC->Log(LOG_ERROR, "Seeking beyond the end position: %I64d > %I64d", m_currentPosition, m_endPosition);
    m_currentPosition = m_endPosition;
  }

  RefreshTSBufferFile();
  return S_OK;
}

int64_t MultiFileReader::GetFilePointer()
{
//  RefreshTSBufferFile();
  return m_currentPosition;
}

long MultiFileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes)
{
  long hr;

  // If the file has already been closed, don't continue
  if (m_TSBufferFile.IsFileInvalid())
    return S_FALSE;

  RefreshTSBufferFile();
  RefreshFileSize();

  if (m_currentPosition < m_startPosition)
  {
    XBMC->Log(LOG_INFO, "%s: current position adjusted from %d to %d.", __FUNCTION__, m_currentPosition, m_startPosition);
    m_currentPosition = m_startPosition;
  }

  // Find out which file the currentPosition is in.
  MultiFileReaderFile *file = NULL;
  std::vector<MultiFileReaderFile *>::iterator it = m_tsFiles.begin();
  for ( ; it < m_tsFiles.end() ; it++ )
  {
    file = *it;
    if (m_currentPosition < (file->startPosition + file->length))
      break;
  };

  // XBMC->Log(LOG_DEBUG, "%s: reading %ld bytes. File %s, start %d, current %d, end %d.", __FUNCTION__, lDataLength, file->filename.c_str(), m_startPosition, m_currentPosition, m_endPosition);


  if(!file)
  {
    XBMC->Log(LOG_ERROR, "MultiFileReader::no file");
    XBMC->QueueNotification(QUEUE_ERROR, "No buffer file");
    return S_FALSE;
  }
  if (m_currentPosition < (file->startPosition + file->length))
  {
    if (m_TSFileId != file->filePositionId)
    {
      m_TSFile.CloseFile();
      m_TSFile.SetFileName(file->filename.c_str());
      m_TSFile.OpenFile();

      m_TSFileId = file->filePositionId;

      if (m_bDebugOutput)
      {
        XBMC->Log(LOG_DEBUG, "MultiFileReader::Read() Current File Changed to %s\n", file->filename.c_str());
      }
    }

    int64_t seekPosition = m_currentPosition - file->startPosition;

    m_TSFile.SetFilePointer(seekPosition, FILE_BEGIN);
    int64_t posSeeked=m_TSFile.GetFilePointer();
    if (posSeeked!=seekPosition)
    {
      XBMC->Log(LOG_ERROR, "SEEK FAILED");
    }

    unsigned long bytesRead = 0;

    int64_t bytesToRead = file->length - seekPosition;
    if (lDataLength > bytesToRead)
    {
      // XBMC->Log(LOG_DEBUG, "%s: datalength %lu bytesToRead %lli.", __FUNCTION__, lDataLength, bytesToRead);
      hr = m_TSFile.Read(pbData, (unsigned long)bytesToRead, &bytesRead);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED1");
      }
      m_currentPosition += bytesToRead;

      hr = this->Read(pbData + bytesToRead, lDataLength - (unsigned long)bytesToRead, dwReadBytes);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED2");
      }
      *dwReadBytes += bytesRead;
    }
    else
    {
      hr = m_TSFile.Read(pbData, lDataLength, dwReadBytes);
      if (FAILED(hr))
      {
        XBMC->Log(LOG_ERROR, "READ FAILED3");
      }
      m_currentPosition += lDataLength;
    }
  }
  else
  {
    // The current position is past the end of the last file
    *dwReadBytes = 0;
  }

  // XBMC->Log(LOG_DEBUG, "%s: read %lu bytes. start %lli, current %lli, end %lli.", __FUNCTION__, *dwReadBytes, m_startPosition, m_currentPosition, m_endPosition);
  return S_OK;
}

long MultiFileReader::Read(unsigned char* pbData, unsigned long lDataLength, unsigned long *dwReadBytes, int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //If end method then we want llDistanceToMove to be the end of the buffer that we read.
  if (dwMoveMethod == FILE_END)
    llDistanceToMove = 0 - llDistanceToMove - lDataLength;

  SetFilePointer(llDistanceToMove, dwMoveMethod);

  return Read(pbData, lDataLength, dwReadBytes);
}

long MultiFileReader::get_ReadOnly(bool *ReadOnly)
{
//  CheckPointer(ReadOnly, E_POINTER);

  if (!m_TSBufferFile.IsFileInvalid())
    return m_TSBufferFile.get_ReadOnly(ReadOnly);

  *ReadOnly = m_bReadOnly;
  return S_OK;
}
        //ensures that there's always a back slash at the end
//        wPathName[wcslen(wPathName)] = char(92*(int)(wPathName[wcslen(wPathName)-1]!=char(92)));

long MultiFileReader::RefreshTSBufferFile()
{
  if (m_TSBufferFile.IsFileInvalid())
    return S_FALSE;

  unsigned long bytesRead;
  MultiFileReaderFile *file;

  long result;
  int64_t currentPosition;
  long filesAdded, filesRemoved;
  long filesAdded2, filesRemoved2;
  long Error;
  long Loop=10;

  //char* pBuffer;
  wchar_t* pBuffer = NULL;

  do
  {
    Error = 0;
    currentPosition = -1;
    filesAdded = -1;
    filesRemoved = -1;
    filesAdded2 = -2;
    filesRemoved2 = -2;

    int64_t fileLength = m_TSBufferFile.GetFileSize();

    // Min file length is Header ( int64_t + long + long ) + filelist ( > 0 ) + Footer ( long + long ) 
    if (fileLength <= (sizeof(int64_t) + sizeof(long) + sizeof(long) + sizeof(wchar_t) + sizeof(long) + sizeof(long)))
    {
      if (m_bDebugOutput)
      {
        XBMC->Log(LOG_DEBUG, "MultiFileReader::RefreshTSBufferFile() TSBufferFile too short");
      }
      return S_FALSE;
    }

    m_TSBufferFile.SetFilePointer(0, FILE_BEGIN);

    unsigned long readLength = sizeof(currentPosition) + sizeof(filesAdded) + sizeof(filesRemoved);
    unsigned char* readBuffer = new unsigned char[readLength];

    result = m_TSBufferFile.Read(readBuffer, readLength, &bytesRead);

    if (!SUCCEEDED(result) || bytesRead!=readLength)
      Error |= 0x02;

    if(Error == 0)
    {
      currentPosition = *((int64_t*)(readBuffer + 0));
		  filesAdded = *((long*)(readBuffer + sizeof(int64_t)));
		  filesRemoved = *((long*)(readBuffer + sizeof(int64_t) + sizeof(long)));
      // XBMC->Log(LOG_DEBUG, "MultiFileReader::RefreshTSBufferFile() currentPosition %lli", currentPosition);
    }

    delete[] readBuffer;

    // If no files added or removed, break the loop !
    if ((m_filesAdded == filesAdded) && (m_filesRemoved == filesRemoved)) 
      break;

    int64_t remainingLength = fileLength - sizeof(int64_t) - sizeof(long) - sizeof(long) - sizeof(long) - sizeof(long) ;

    // Above 100kb seems stupid and figure out a problem !!!
    if (remainingLength > 100000)
      Error=0x10;;
  
    pBuffer = (wchar_t*) new char[(unsigned int)remainingLength];

    result=m_TSBufferFile.Read((unsigned char*)pBuffer, (ULONG)remainingLength, &bytesRead);
    if (!SUCCEEDED(result)||  bytesRead != remainingLength) Error=0x20;

    //unsigned char* pb = (unsigned char*) pBuffer;
    //for (unsigned long i = 0; i < bytesRead; i++)
    //{
    //  XBMC->Log(LOG_DEBUG, "%s: pBuffer byte[%d] == %x.", __FUNCTION__, i, pb[i]);
    //}

    readLength = sizeof(filesAdded) + sizeof(filesRemoved);

    readBuffer = new unsigned char[readLength];

    result = m_TSBufferFile.Read(readBuffer, readLength, &bytesRead);

    if (!SUCCEEDED(result) || bytesRead != readLength) 
      Error |= 0x40;

    if(Error == 0)
	  {
		  filesAdded2 = *((long*)(readBuffer + 0));
		  filesRemoved2 = *((long*)(readBuffer + sizeof(long)));
	  }

    delete[] readBuffer;

    if ((filesAdded2 != filesAdded) || (filesRemoved2 != filesRemoved))
    {
      Error |= 0x80;

      XBMC->Log(LOG_ERROR, "MultiFileReader has error 0x%x in Loop %d. Try to clear SMB Cache.", Error, 10-Loop);
      XBMC->Log(LOG_DEBUG, "%s: filesAdded %d, filesAdded2 %d, filesRemoved %d, filesRemoved2 %d.", __FUNCTION__, filesAdded, filesAdded2, filesRemoved, filesRemoved2);

      // try to clear local / remote SMB file cache. This should happen when we close the filehandle
      m_TSBufferFile.CloseFile();
      m_TSBufferFile.OpenFile();
      Sleep(5);
    }

    if (Error) delete[] pBuffer;

    Loop-- ;
  } while ( Error && Loop ) ; // If Error is set, try again...until Loop reaches 0.
 
  if (Loop < 8)
  {
    XBMC->Log(LOG_DEBUG, "MultiFileReader has waited %d times for TSbuffer integrity.", 10-Loop) ;

    if(Error)
    {
      XBMC->Log(LOG_ERROR, "MultiFileReader has failed for TSbuffer integrity. Error : %x", Error) ;
      return E_FAIL ;
    }
  }

  //randomly park the file pointer to help minimise HDD clogging
  if(currentPosition&1)
    m_TSBufferFile.SetFilePointer(0, FILE_BEGIN);
  else
    m_TSBufferFile.SetFilePointer(0, FILE_END);

  if ((m_filesAdded != filesAdded) || (m_filesRemoved != filesRemoved))
  {
    long filesToRemove = filesRemoved - m_filesRemoved;
    long filesToAdd = filesAdded - m_filesAdded;
    long fileID = filesRemoved;
    int64_t nextStartPosition = 0;

    if (m_bDebugOutput)
    {
      XBMC->Log(LOG_DEBUG, "MultiFileReader: Files Added %i, Removed %i\n", filesToAdd, filesToRemove);
    }

    // Removed files that aren't present anymore.
    while ((filesToRemove > 0) && (m_tsFiles.size() > 0))
    {
      MultiFileReaderFile *file = m_tsFiles.at(0);

      if (m_bDebugOutput)
      {
        XBMC->Log(LOG_DEBUG, "MultiFileReader: Removing file %s\n", file->filename.c_str());
      }
      
      delete file;
      m_tsFiles.erase(m_tsFiles.begin());

      filesToRemove--;
    }


    // Figure out what the start position of the next new file will be
    if (m_tsFiles.size() > 0)
    {
      file = m_tsFiles.back();

      if (filesToAdd > 0)
      {
        // If we're adding files the changes are the one at the back has a partial length
        // so we need update it.
        if (m_bDebugOutput)
          GetFileLength(file->filename.c_str(), file->length);
        else
          GetFileLength(file->filename.c_str(), file->length);
      }

      nextStartPosition = file->startPosition + file->length;
    }

    // Get the real path of the buffer file
    char* filename;
    std::string sFilename;
    std::string path;
    size_t pos = std::string::npos;

    m_TSBufferFile.GetFileName(&filename);
    sFilename = filename;
    pos = sFilename.find_last_of(PATH_SEPARATOR_CHAR);
    path = sFilename.substr(0, pos+1);
    //name3 = filename1.substr(pos+1);

    // Create a list of files in the .tsbuffer file.
    std::vector<std::string> filenames;

    wchar_t* pwCurrFile = pBuffer;    //Get a pointer to the first wchar filename string in pBuffer
    long length = WcsLen(pwCurrFile);

    //XBMC->Log(LOG_DEBUG, "%s: WcsLen(%d), sizeof(wchar_t) == %d.", __FUNCTION__, length, sizeof(wchar_t));

    while(length > 0)
    {
      // Convert the current filename (wchar to normal char)
      char* wide2normal = new char[length + 1];
      WcsToMbs( wide2normal, pwCurrFile, length);
      wide2normal[length] = '\0';

      //unsigned char* pb = (unsigned char*) wide2normal;
      //for (unsigned long i = 0; i < rc; i++)
      //{
      //  XBMC->Log(LOG_DEBUG, "%s: pBuffer byte[%d] == %x.", __FUNCTION__, i, pb[i]);
      //}

      std::string sCurrFile = wide2normal;
      //XBMC->Log(LOG_DEBUG, "%s: filename %s (%s).", __FUNCTION__, wide2normal, sCurrFile.c_str());
      delete[] wide2normal;


      // Modify filename path here to include the real (local) path
      pos = sCurrFile.find_last_of(92);
      std::string name = sCurrFile.substr(pos+1);
      if (path.length()>0 && name.length()>0)
      {
        // Replace the original path with our local path
        filenames.push_back(path + name);
      }
      else
      {
        // Keep existing path
        filenames.push_back(sCurrFile);
      }
      
      // Move the wchar buffer pointer to the next wchar string
#if defined(TARGET_WINDOWS)
      pwCurrFile += (length + 1);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
      unsigned short *pus = (unsigned short *) pwCurrFile;
      pus += (length + 1);
      pwCurrFile = (wchar_t *) pus;
#else
#error Implement pointer adjustment for 16-bit wchars for your OS!
#endif
      length = WcsLen(pwCurrFile);
    }

    // Go through files
    std::vector<MultiFileReaderFile *>::iterator itFiles = m_tsFiles.begin();
    //std::vector<char*>::iterator itFilenames = filenames.begin();
    std::vector<std::string>::iterator itFilenames = filenames.begin();

    while (itFiles < m_tsFiles.end())
    {
      file = *itFiles;

      itFiles++;
      fileID++;

      if (itFilenames < filenames.end())
      {
        // TODO: Check that the filenames match. ( Ambass : With buffer integrity check, probably no need to do this !)
        itFilenames++;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "MultiFileReader: Missing files!!\n");
      }
    }

    while (itFilenames < filenames.end())
    {
      std::string pFilename = *itFilenames;

      if (m_bDebugOutput)
      {
        int nextStPos = (int)nextStartPosition;
        XBMC->Log(LOG_DEBUG, "MultiFileReader: Adding file %s (%i)\n", pFilename.c_str(), nextStPos);
      }

      file = new MultiFileReaderFile();
      file->filename = pFilename;
      file->startPosition = nextStartPosition;

      fileID++;
      file->filePositionId = fileID;

      GetFileLength(file->filename.c_str(), file->length);

      m_tsFiles.push_back(file);

      nextStartPosition = file->startPosition + file->length;

      itFilenames++;
    }

    m_filesAdded = filesAdded;
    m_filesRemoved = filesRemoved;

    delete[] pBuffer;
  }

  if (m_tsFiles.size() > 0)
  {
    file = m_tsFiles.front();
    m_startPosition = file->startPosition;

    file = m_tsFiles.back();
    file->length = currentPosition;
    m_endPosition = file->startPosition + currentPosition;
  
    if (m_bDebugOutput)
    {
      int64_t stPos = m_startPosition;
      int64_t endPos = m_endPosition;
      int64_t curPos = m_currentPosition;
      XBMC->Log(LOG_DEBUG, "StartPosition %lli, EndPosition %lli, CurrentPosition %lli\n", stPos, endPos, curPos);
    }
    // int64_t stPos = m_startPosition;
    // int64_t endPos = m_endPosition;
    // int64_t curPos = m_currentPosition;
    // XBMC->Log(LOG_DEBUG, "StartPosition %lli, EndPosition %lli, CurrentPosition %lli\n", stPos, endPos, curPos);
  }
  else
  {
    m_startPosition = 0;
    m_endPosition = 0;
  }

  return S_OK;
}

//TODO: make OS independent. Currently Windows specific
long MultiFileReader::GetFileLength(const char* pFilename, int64_t &length)
{
#if defined(TARGET_WINDOWS)
  //USES_CONVERSION;

  length = 0;

  // Try to open the file
  HANDLE hFile = ::CreateFile(pFilename,   // The filename
            (DWORD) GENERIC_READ,          // File access
             (DWORD) (FILE_SHARE_READ |
             FILE_SHARE_WRITE),            // Share access
             NULL,                         // Security
             (DWORD) OPEN_EXISTING,        // Open flags
             (DWORD) 0,                    // More flags
             NULL);                        // Template
  if (hFile != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER li;
    li.QuadPart = 0;
    li.LowPart = ::SetFilePointer(hFile, 0, &li.HighPart, FILE_END);
    ::CloseHandle(hFile);
    
    length = li.QuadPart;
  }
  else
  {
    //wchar_t msg[MAX_PATH];
    DWORD dwErr = GetLastError();
    //swprintf((LPWSTR)&msg, L"Failed to open file %s : 0x%x\n", pFilename, dwErr);
    //::OutputDebugString(W2T((LPWSTR)&msg));
    XBMC->Log(LOG_ERROR, "Failed to open file %s : 0x%x\n", pFilename, dwErr);
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to open file %s", pFilename);
    return HRESULT_FROM_WIN32(dwErr);
  }
  return S_OK;
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  //USES_CONVERSION;

  length = 0;

  // Try to open the file
  CFile hFile;
  struct stat64 filestatus;
  if (hFile.Open(pFilename) && hFile.Stat(&filestatus) >= 0)
  {
    length = filestatus.st_size;
    hFile.Close();
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Failed to open file %s : 0x%x(%s)\n", pFilename, errno, strerror(errno));
    XBMC->QueueNotification(QUEUE_ERROR, "Failed to open file %s", pFilename);
    return S_FALSE;
  }
  return S_OK;
#else
#error FIXME: Add MultiFileReader::GetFileLenght implementation for your OS.
  return S_FALSE;
#endif
}

long MultiFileReader::get_DelayMode(bool *DelayMode)
{
  *DelayMode = m_bDelay;
  return S_OK;
}

long MultiFileReader::set_DelayMode(bool DelayMode)
{
  m_bDelay = DelayMode;
  return S_OK;
}

long MultiFileReader::get_ReaderMode(unsigned short *ReaderMode)
{
  *ReaderMode = TRUE;
  return S_OK;
}

unsigned long MultiFileReader::setFilePointer(int64_t llDistanceToMove, unsigned long dwMoveMethod)
{
  //Get the file information
  int64_t fileStart, fileEnd, fileLength;
  GetFileSize(&fileStart, &fileLength);
  fileEnd = (int64_t)(fileLength + fileStart);
  if (dwMoveMethod == FILE_BEGIN)
  {
    return SetFilePointer((int64_t)min(fileEnd,(int64_t)(llDistanceToMove + fileStart)), FILE_BEGIN);
  }
  else
  {
    return SetFilePointer((int64_t)max((int64_t)-fileLength, llDistanceToMove), FILE_END);
  }
}

int64_t MultiFileReader::getFilePointer()
{
  int64_t fileStart, fileEnd, fileLength;

  GetFileSize(&fileStart, &fileLength);
  fileEnd = fileLength + fileStart;

  return (int64_t)(GetFilePointer() - fileStart);
}

int64_t MultiFileReader::getBufferPointer()
{
  return m_llBufferPointer;
}

void MultiFileReader::setBufferPointer()
{
  m_llBufferPointer = getFilePointer();
}


int64_t MultiFileReader::GetFileSize()
{
  RefreshTSBufferFile();
  return m_endPosition - m_startPosition;
  //if (m_cachedFileSize==0)
  //{
  //  RefreshTSBufferFile();
  //  RefreshFileSize();
  //}
  //return m_cachedFileSize;
}

void MultiFileReader::RefreshFileSize()
{
  int64_t fileLength = 0;
  std::vector<MultiFileReaderFile *>::iterator it = m_tsFiles.begin();
  for ( ; it < m_tsFiles.end() ; it++ )
  {
    MultiFileReaderFile *file =*it;
    fileLength+=file->length;
  }
  m_cachedFileSize= fileLength;
  // XBMC->Log(LOG_DEBUG, "%s: m_cachedFileSize %d.", __FUNCTION__, m_cachedFileSize);
}

// The ts.tsbuffer file will contain 'Windows' wchars which are
// 16 bit each, this is the platform independent wcslen
size_t MultiFileReader::WcsLen(const void *str)
{
#if defined(TARGET_WINDOWS)
  return wcslen((const wchar_t *)str);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  const unsigned short *eos = (const unsigned short*)str;
  while( *eos++ ) ;
  return( (size_t)(eos - (const unsigned short*)str) -1);
#else
#endif
}

// The ts.tsbuffer file will contain 'Windows' wchars which are
// 16 bit each, this is the platform independent wcstombs
size_t MultiFileReader::WcsToMbs(char *s, const void *w, size_t n)
{
#if defined(TARGET_WINDOWS)
  return wcstombs(s, (const wchar_t *)w, n);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
  size_t i = 0;
  const unsigned short *wc = (const unsigned short*) w;
  while(wc[i] && (i < n))
  {
    s[i] = wc[i];
    ++i;
  }
  if (i < n) s[i] = '\0';

  return (i);
#else
#endif
}
#endif //TSREADER
