#include "XFileUtils.h"
#include "XTimeUtils.h"
#include "Util.h"

#ifdef _LINUX

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h> 
#include <regex.h>
#include <dirent.h>
#include <errno.h>

#include "cdioSupport.h"

#include "../utils/log.h"

HANDLE FindFirstFile(LPCSTR szPath,LPWIN32_FIND_DATA lpFindData) {
	if (lpFindData == NULL || szPath == NULL)
		return NULL;

	CStdString strPath(szPath);

	if (strPath.empty())
		return INVALID_HANDLE_VALUE;

        strPath.Replace("\\","/");

	// if the file name is a directory then we add a * to look for all files in this directory
	DIR *testDir = opendir(szPath);
	if (testDir) {
		strPath += "/*";
		closedir(testDir);
	}

	int nFilePos = strPath.ReverseFind(XBMC_FILE_SEP);
	
	CStdString strDir = ".";
	CStdString strFiles = strPath;

	if (nFilePos > 0) {
		strDir = strPath.substr(0,nFilePos);
		strFiles = strPath.substr(nFilePos + 1);
	}

        if (strFiles == "*.*")
           strFiles = "*";

	strFiles = CStdString("^") + strFiles + "$";
	strFiles.Replace(".","\\.");
	strFiles.Replace("*",".*");
	strFiles.Replace("?",".");

	strFiles.MakeLower();

	int status;
	regex_t re;
	if (regcomp(&re, strFiles, REG_EXTENDED|REG_NOSUB) != 0) {
		return(INVALID_HANDLE_VALUE);      
	}

	struct dirent **namelist = NULL;
	int n = scandir(strDir, &namelist, 0, alphasort);

	CXHandle *pHandle = new CXHandle(CXHandle::HND_FIND_FILE);
		pHandle->m_FindFileDir = strDir;

	while (n-- > 0) {
		CStdString strComp(namelist[n]->d_name);
		strComp.MakeLower();

		status = regexec(&re, strComp.c_str(), (size_t) 0, NULL, 0);
		if (status == 0) {
			pHandle->m_FindFileResults.push_back(namelist[n]->d_name);
		}
		free(namelist[n]);
	}

	if (namelist)
		free(namelist);

	regfree(&re);

	if (pHandle->m_FindFileResults.size() == 0) {
		delete pHandle;
		return INVALID_HANDLE_VALUE;
	}

	FindNextFile(pHandle, lpFindData);

	return pHandle;
}

BOOL   FindNextFile(HANDLE hHandle, LPWIN32_FIND_DATA lpFindData) {
	if (lpFindData == NULL || hHandle == NULL || hHandle->GetType() != CXHandle::HND_FIND_FILE)
		return FALSE;

	if ((unsigned int) hHandle->m_nFindFileIterator >= hHandle->m_FindFileResults.size())
		return FALSE;

	CStdString strFileName = hHandle->m_FindFileResults[hHandle->m_nFindFileIterator++];
        CStdString strFileNameTest = hHandle->m_FindFileDir + '/' + strFileName;
	
	struct stat64 fileStat;
	if (stat64(strFileNameTest, &fileStat) != 0)
		return FALSE;

	bool bIsDir = false;
	DIR *testDir = opendir(strFileNameTest);
	if (testDir) {
		bIsDir = true;
		closedir(testDir);
	}

	memset(lpFindData,0,sizeof(WIN32_FIND_DATA));
	
	lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	strcpy(lpFindData->cFileName, strFileName.c_str());

	if (bIsDir)
		lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (strFileName[0] == '.')
		lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

	if (access(strFileName, R_OK) == 0 && access(strFileName, W_OK) != 0)
		lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

	TimeTToFileTime(fileStat.st_ctime, &lpFindData->ftCreationTime);
	TimeTToFileTime(fileStat.st_atime, &lpFindData->ftLastAccessTime);
	TimeTToFileTime(fileStat.st_mtime, &lpFindData->ftLastWriteTime);
	
	lpFindData->nFileSizeHigh = (DWORD)(fileStat.st_size >> 32);
	lpFindData->nFileSizeLow =  (DWORD)fileStat.st_size;

	return TRUE;
}

BOOL   FindClose(HANDLE hFindFile) {
	return CloseHandle(hFindFile);
}

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
  DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  // Fail on unsupported items
  if (lpSecurityAttributes != NULL )
  {
    CLog::Log(LOGERROR, "CreateFile does not support security attributes");
    return INVALID_HANDLE_VALUE;
  } 
  
  if (hTemplateFile != (HANDLE) 0)
  {
    CLog::Log(LOGERROR, "CreateFile does not support template file");
    return INVALID_HANDLE_VALUE;
  }
  
  int flags = 0, mode=S_IRUSR;
  if (dwDesiredAccess & FILE_WRITE_DATA) {
    flags = O_RDWR;
    mode |= S_IWUSR;
  }
  else if ( (dwDesiredAccess & FILE_READ_DATA) == FILE_READ_DATA) 
    flags = O_RDONLY;
  else
  {
    CLog::Log(LOGERROR, "CreateFile does not permit access other than read and/or write");
    return INVALID_HANDLE_VALUE;
  }

  switch (dwCreationDisposition)
  {
    case OPEN_ALWAYS:
      flags |= O_CREAT;
      break;
    case TRUNCATE_EXISTING:
      flags |= O_TRUNC;
      mode  |= S_IWUSR;
      break;
    case CREATE_ALWAYS:
      flags |= O_CREAT|O_TRUNC;
      mode  |= S_IWUSR;
      break;
    case CREATE_NEW:
      flags |= O_CREAT|O_TRUNC|O_EXCL;
      mode  |= S_IWUSR;
      break;
    case OPEN_EXISTING:
      break;
  }
  
  int fd = 0;
  bool cd = false;

  if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    flags |= O_SYNC;

  CStdString strResultFile(lpFileName);

  // special case for opening the cdrom device
  if (strcmp(lpFileName, MEDIA_DETECT::CCdIoSupport::GetDeviceFileName())==0)
  {
    fd = open(lpFileName, O_RDONLY | O_NONBLOCK);
    cd = true;

  }
  else
  {
    fd = open(lpFileName, flags, mode);

    // Important to check reason for fail. Only if its 
    // "file does not exist" shall we try to find the file
    if (fd == -1 && errno == ENOENT) 
    {
      // Failed to open file. maybe due to case sensitivity. 
      // Try opening the same name in lower case.
      CStdString igFileName = PTH_IC(lpFileName);
      fd = open(igFileName.c_str(), flags, mode);
      if (fd != -1) 
      {
        CLog::Log(LOGWARNING,"%s, successfuly opened <%s> instead of <%s>", __FUNCTION__, igFileName.c_str(), lpFileName);
        strResultFile = igFileName;
      }
    }
  }
  
  if (fd == -1)
  {
    CLog::Log(LOGWARNING,"%s, error %d opening file <%s>. ", __FUNCTION__, errno, lpFileName);
    return INVALID_HANDLE_VALUE;
  }

  HANDLE result = new CXHandle(CXHandle::HND_FILE);
  if (cd)
  {
    result->m_bCDROM = true;
  }
  else
  {
    result->m_bCDROM = false;
  }
    
  result->fd = fd;

  // if FILE_FLAG_DELETE_ON_CLOSE then "unlink" the file (delete)
  // the file will be deleted when the last open descriptor is closed.  
  if (dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
	unlink(strResultFile);

  return result;
}

BOOL DeleteFile(LPCTSTR lpFileName)
{
  if (unlink(lpFileName) == 0)
    return 1;
  
  if (errno == EACCES) {
    CLog::Log(LOGWARNING,"%s - cant delete file, trying to change mode <%s>", __FUNCTION__, lpFileName);
    if (chmod(lpFileName, 0600) != 0) {
      CLog::Log(LOGWARNING,"%s - failed to change mode <%s>", __FUNCTION__, lpFileName);
      return 0;
    }

    CLog::Log(LOGDEBUG,"%s - reattempt to delete file",__FUNCTION__);

    if (unlink(lpFileName) == 0)
      return 1;
  }
  else if (errno == ENOENT) {
    CStdString strLower(lpFileName);
    strLower.MakeLower();
    CLog::Log(LOGWARNING,"%s - cant delete file <%s>. trying lower case <%s>", __FUNCTION__, lpFileName, strLower.c_str());
    if (unlink(strLower.c_str()) == 0) {
      CLog::Log(LOGDEBUG,"%s - successfuly removed file <%s>", __FUNCTION__, strLower.c_str());
      return 1;
    }
  }

  return 0;
}

BOOL MoveFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName) 
{
  if (rename(lpExistingFileName, lpNewFileName) == 0)
    return 1;

  if (errno == EACCES) {
    CLog::Log(LOGWARNING,"%s - cant move file, trying to change mode <%s>", __FUNCTION__, lpExistingFileName);
    if (chmod(lpExistingFileName, 0600) != 0) {
      CLog::Log(LOGWARNING,"%s - failed to change mode <%s>", __FUNCTION__, lpExistingFileName);
      return 0;
    }

    CLog::Log(LOGDEBUG,"%s - reattempt to move file",__FUNCTION__);

    if (rename(lpExistingFileName, lpNewFileName) == 0)
      return 1;
  }
  else if (errno == ENOENT) {
    CStdString strLower(lpExistingFileName);
    strLower.MakeLower();
    CLog::Log(LOGWARNING,"%s - cant move file <%s>. trying lower case <%s>", __FUNCTION__, lpExistingFileName, strLower.c_str());
    if (rename(strLower.c_str(), lpNewFileName) == 0) {
      CLog::Log(LOGDEBUG,"%s - successfuly moved file <%s>", __FUNCTION__, strLower.c_str());
      return 1;
    }
  }

  return 0;
}

BOOL CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
  // If the destination file exists and we should fail...guess what? we fail!
  struct stat destStat;
  bool isDestExists = (stat(lpNewFileName, &destStat) == 0);
  if (isDestExists && bFailIfExists) 
  {
    return 0;
  }
  
  CStdString strResultFile(lpExistingFileName);

  // Open the files
  int sf = open(lpExistingFileName, O_RDONLY);
  if (sf == -1 && errno == ENOENT) // important to check reason for fail. only if its "file does not exist" shall we try lower case.
  {
    CStdString strLower(lpExistingFileName);
    strLower.MakeLower();

    // failed to open file. maybe due to case sensitivity. try opening the same name in lower case.
    CLog::Log(LOGWARNING,"%s, cant open file <%s>. trying to use lowercase <%s>", __FUNCTION__, lpExistingFileName, strLower.c_str());
    sf = open(strLower.c_str(), O_RDONLY);
    if (sf != -1) {
      CLog::Log(LOGDEBUG,"%s, successfuly opened <%s>", __FUNCTION__, strLower.c_str());
      strResultFile = strLower;
    }
  }

  if (sf == -1) 
  {
    CLog::Log(LOGERROR,"%s - cant open source file <%s>", __FUNCTION__, lpExistingFileName);
    return 0;
  }
    
  int df = open(lpNewFileName, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
  if (df == -1)
  {
    if (errno == EACCES) {
      CLog::Log(LOGWARNING,"%s - cant write to dest file, trying to change mode <%s>", __FUNCTION__, lpNewFileName);
      if (chmod(lpNewFileName, 0600) != 0) {
        CLog::Log(LOGWARNING,"%s - failed to change mode <%s>", __FUNCTION__, lpNewFileName);
        close(sf);
        return 0;
      }

      CLog::Log(LOGDEBUG,"%s - reattempt to open dest file",__FUNCTION__);

      df = open(lpNewFileName, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
      if (df == -1)
      {
        CLog::Log(LOGERROR,"%s - cant open dest file <%s>", __FUNCTION__, lpNewFileName);
        close(sf);
        return 0;
      }

      CLog::Log(LOGDEBUG,"%s - successfuly opened dest file",__FUNCTION__);

    }
  }
  
  // Read and write chunks of 16K
  char buf[16384];
  long long bytesRead = 1;
  long long bytesWritten = 1;
  
  while (bytesRead > 0 && bytesWritten > 0)
  {
    bytesRead = read(sf, buf, sizeof(buf));
    if (bytesRead > 0)
      bytesWritten = write(df, buf, bytesRead);       
  }
  
  // Done
  close(sf);
  close(df);
  
  if (bytesRead == -1 || bytesWritten == -1)
    return 0;
    
  return 1;    
}
		
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped)
{
  if (lpOverlapped)
  {
    CLog::Log(LOGERROR, "ReadFile does not support overlapped I/O");
    return 0;
 }
  
  size_t bytesRead = read(hFile->fd, lpBuffer, nNumberOfBytesToRead);
  if (bytesRead == (size_t) -1)
    return 0;
    
  if (lpNumberOfBytesRead)
    *lpNumberOfBytesRead = bytesRead;

  return 1;
}

BOOL WriteFile(HANDLE hFile, const void * lpBuffer, DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped)
{
  if (lpOverlapped)
  {
    CLog::Log(LOGERROR, "ReadFile does not support overlapped I/O");
    return 0;
 }
  
  size_t bytesWritten = write(hFile->fd, lpBuffer, nNumberOfBytesToWrite);

  if (bytesWritten == (size_t) -1)
    return 0;
    
  *lpNumberOfBytesWritten = bytesWritten;

  return 1;
}

BOOL   CreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) 
{
  if (mkdir(lpPathName, 0755) == 0)
    return 1;

  if (errno == ENOENT) {
    CLog::Log(LOGWARNING,"%s, cant create dir <%s>. trying lower case.", __FUNCTION__, lpPathName);
    CStdString strLower(lpPathName);
    strLower.MakeLower();

    if (mkdir(strLower.c_str(), 0755) == 0)
      return 1;
  }

  return 0;
}

BOOL   RemoveDirectory(LPCTSTR lpPathName) 
{
  if (rmdir(lpPathName) == 0)
    return 1;

  if (errno == ENOENT) {
    CLog::Log(LOGWARNING,"%s, cant remove dir <%s>. trying lower case.", __FUNCTION__, lpPathName);
    CStdString strLower(lpPathName);
    strLower.MakeLower();

    if (rmdir(strLower.c_str()) == 0)
      return 1;
  }
  return 0;
}

DWORD  SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {

	if (hFile == NULL)
		return 0;

	LONGLONG offset = lDistanceToMove;
	if (lpDistanceToMoveHigh) {
		LONGLONG helper = *lpDistanceToMoveHigh;
		helper <<= 32;
		offset |= helper;
	}

	int nMode = SEEK_SET;
	if (dwMoveMethod == FILE_CURRENT)
		nMode = SEEK_CUR;
	else if (dwMoveMethod == FILE_END)
		nMode = SEEK_END;

	off64_t currOff;
	if (hFile->m_bCDROM)
	{
	  currOff = lseek64(hFile->fd, offset, nMode);	  
	  currOff = offset;
	}
	else
	{
	  currOff = lseek64(hFile->fd, offset, nMode);	  
	}
	
	if (lpDistanceToMoveHigh) {
		*lpDistanceToMoveHigh = (LONG)(currOff >> 32);
	}
	
	return (DWORD)currOff;
}

// uses statfs
BOOL GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes
  ) 

{
	struct statfs64 fsInfo;
	if (statfs64(lpDirectoryName, &fsInfo) != 0)
		return false;

	if (lpFreeBytesAvailable)
		lpFreeBytesAvailable->QuadPart =  fsInfo.f_bavail * fsInfo.f_bsize;

	if (lpTotalNumberOfBytes)
		lpTotalNumberOfBytes->QuadPart = fsInfo.f_blocks * fsInfo.f_bsize;

	if (lpTotalNumberOfFreeBytes)
		lpTotalNumberOfFreeBytes->QuadPart = fsInfo.f_bfree * fsInfo.f_bsize;

	return true;
}

DWORD GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation ) {
  if (lpTimeZoneInformation == NULL)
    return TIME_ZONE_ID_UNKNOWN;

  lpTimeZoneInformation->Bias = timezone / 60;
  swprintf(lpTimeZoneInformation->StandardName, 31, L"%s", tzname[0]);
  swprintf(lpTimeZoneInformation->DaylightName, 31, L"%s", tzname[1]);

  return 1;
}

BOOL SetEndOfFile(HANDLE hFile) {
	if (hFile == NULL)
		return false;

	// get the current offset
	off64_t currOff = lseek64(hFile->fd, 0, SEEK_CUR);
	ftruncate(hFile->fd, currOff);
	return true;
}

DWORD SleepEx( DWORD dwMilliseconds,  BOOL bAlertable) {
#warning need to complete function SleepEx
	SDL_Delay(dwMilliseconds);
	return 0;
}

BOOL SetFilePointerEx(  HANDLE hFile,
						LARGE_INTEGER liDistanceToMove,
						PLARGE_INTEGER lpNewFilePointer,
						DWORD dwMoveMethod ) {

	int nMode = SEEK_SET;
	if (dwMoveMethod == FILE_CURRENT)
		nMode = SEEK_CUR;
	else if (dwMoveMethod == FILE_END)
		nMode = SEEK_END;

	off64_t toMove = liDistanceToMove.QuadPart;
	off64_t currOff = lseek64(hFile->fd, toMove, nMode);

	if (lpNewFilePointer)
		lpNewFilePointer->QuadPart = currOff;

	return true;
}

BOOL GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize) {
	if (hFile == NULL || lpFileSize == NULL) {
		return false;
	}

	
	struct stat64 fileStat;
	if (fstat64(hFile->fd, &fileStat) != 0)
		return false;
	
	lpFileSize->QuadPart = fileStat.st_size;
	return true;
}

BOOL FlushFileBuffers( HANDLE hFile ) {
	if (hFile == NULL) {
		return 0;
	}

	return (fsync(hFile->fd) == 0);
}

int _stat64(   const char *path,   struct __stat64 *buffer ) {

	if (buffer == NULL || path == NULL)
		return -1;

	struct stat64 buf;
	if ( stat64(path, &buf) != 0 )
		return -1;

	buffer->st_dev = buf.st_dev;
	buffer->st_ino = buf.st_ino;
	buffer->st_mode = buf.st_mode;
	buffer->st_nlink = buf.st_nlink;
	buffer->st_uid = buf.st_uid;
	buffer->st_gid = buf.st_gid;
	buffer->st_rdev = buf.st_rdev;
	buffer->st_size = buf.st_size;
	buffer->_st_atime = buf.st_atime;
	buffer->_st_mtime = buf.st_mtime;
	buffer->_st_ctime = buf.st_ctime;

	return 0;
}

DWORD  GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
	if (hFile == NULL) {
		return 0;
	}

	
	struct stat64 fileStat;
	if (fstat64(hFile->fd, &fileStat) != 0)
		return 0;

	if (lpFileSizeHigh) {
		*lpFileSizeHigh = (DWORD)(fileStat.st_size >> 32); 
	}

	return (DWORD)fileStat.st_size;
}

DWORD  GetFileAttributes(LPCTSTR lpFileName)
{
	if (lpFileName == NULL) {
		return 0;
	}

	DWORD dwAttr = FILE_ATTRIBUTE_NORMAL;
	DIR *tmpDir = opendir(lpFileName);
	if (tmpDir) {
		dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
		closedir(tmpDir); 
	}	

	if (lpFileName[0] == '.')
		dwAttr |= FILE_ATTRIBUTE_HIDDEN;

	if (access(lpFileName, R_OK) == 0 && access(lpFileName, W_OK) != 0)
		dwAttr |= FILE_ATTRIBUTE_READONLY;

	return 0;
}

DWORD  GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer)
{
	if (lpBuffer == NULL)
		return 0;

	if (getcwd(lpBuffer,nBufferLength) == NULL)
		return 0;

  	return strlen(lpBuffer);
}
#endif

