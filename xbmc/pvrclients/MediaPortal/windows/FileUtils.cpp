#include "FileUtils.h"
#include "os_windows.h"
#include <string>

namespace OS
{
  bool CFile::Exists(const std::string& strFileName)
  {
    DWORD dwAttr = GetFileAttributes(strFileName.c_str());

    if(dwAttr == 0xffffffff)
    {
      DWORD dwError = GetLastError();
      if(dwError == ERROR_FILE_NOT_FOUND)
      {
        // file not found
        return false;
      }
      else if(dwError == ERROR_PATH_NOT_FOUND)
      {
        // path not found
        return false;
      }
      else if(dwError == ERROR_ACCESS_DENIED)
      {
        // file or directory exists, but access is denied
        return false;
      }
      else
      {
        // some other error has occured
        return false;
      }
    }
    else
    {
      if(dwAttr & FILE_ATTRIBUTE_DIRECTORY)
      {
        return true;
/*
        // this is a directory
        if(dwAttr & FILE_ATTRIBUTE_ARCHIVE)
          // Directory is archive file
        if(dwAttr & FILE_ATTRIBUTE_COMPRESSED)
          // Directory is compressed
        if(dwAttr & FILE_ATTRIBUTE_ENCRYPTED)
          // Directory is encrypted
        if(dwAttr & FILE_ATTRIBUTE_HIDDEN)
          // Directory is hidden
        if(dwAttr & FILE_ATTRIBUTE_READONLY)
          // Directory is read-only
        if(dwAttr & FILE_ATTRIBUTE_REPARSE_POINT)
          // Directory has an associated reparse point
        if(dwAttr & FILE_ATTRIBUTE_SYSTEM)
          // Directory is part or used exclusively by the operating system
*/
      }
      else
      {
        return true;
/*
        // this is an ordinary file
        if(dwAttr & FILE_ATTRIBUTE_ARCHIVE)
          // File is archive file
        if(dwAttr & FILE_ATTRIBUTE_COMPRESSED)
          // File is compressed
        if(dwAttr & FILE_ATTRIBUTE_ENCRYPTED)
          // File is encrypted
        if(dwAttr & FILE_ATTRIBUTE_HIDDEN)
          // File is hidden
        if(dwAttr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
          // File will not be indexed
        if(dwAttr & FILE_ATTRIBUTE_OFFLINE)
          // Data of file is not immediately available
        if(dwAttr & FILE_ATTRIBUTE_READONLY)
          // File is read-only
        if(dwAttr & FILE_ATTRIBUTE_REPARSE_POINT)
          // File has an associated reparse point
        if(dwAttr & FILE_ATTRIBUTE_SPARSE_FILE)
          // File is a sparse file
        if(dwAttr & FILE_ATTRIBUTE_SYSTEM)
          // File is part or used exclusively by the operating system
        if(dwAttr & FILE_ATTRIBUTE_TEMPORARY)
          // File is being used for temporary storage
*/
       }
    }

    return true;
  }
}